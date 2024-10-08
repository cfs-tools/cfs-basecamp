#!/usr/bin/env python
"""
    Copyright 2022 bitValence, Inc.
    All Rights Reserved.

    This program is free software; you can modify and/or redistribute it
    under the terms of the GNU Affero General Public License
    as published by the Free Software Foundation; version 3 with
    attribution addendums as found in the LICENSE.txt.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    This program may also be used under the terms of a commercial or enterprise
    edition license of Basecamp if purchased from the copyright holder.

    Purpose:
        Provide classes that manage the creation of target projects. Target
        projects automated the process of downloading apps and building a 
        new cFS target to implement a cFS target.        
"""

import sys
sys.path.append("..")
import time
import os
import errno
import json
import configparser
import shutil
from datetime import datetime
from time import sleep
import requests

import logging
logger = logging.getLogger(__name__)

if __name__ == '__main__':
    from jsonfile   import JsonFile
    from utils      import compress_abs_path
    from githubapps import GitHubApps
    from cfstarget  import ManageCfs
else:
    from .jsonfile   import JsonFile
    from .utils      import compress_abs_path
    from .githubapps import GitHubApps
    from .cfstarget  import ManageCfs

from tools import PySimpleGUI_License
import PySimpleGUI as sg

PROJECT_JSON_FILE = 'project.json'

###############################################################################

class ProjectTemplateJson(JsonFile):
    """
    Manage project template JSON files.
    """
    def __init__(self, json_file):
        super().__init__(json_file)
        
    def version(self):
        return self.json['version']

    def reset_child(self):
        pass
        
    def app_list(self):
        return self.json['app-list'].split(',')
                  
    def popup_instructions(self):
        instructions = ''
        try:
           instructions = self.json['popup-instructions']
        except:
           pass
        return  instructions


###############################################################################

class ProjectTemplate():
    """
    """
    def __init__(self, template_path, git_url, usr_app_rel_path, manage_cfs):

        self.path = template_path
        self.json = ProjectTemplateJson(os.path.join(template_path, PROJECT_JSON_FILE))
 
        self.git_url = git_url
        self.usr_app_rel_path = usr_app_rel_path
        
        self.manage_cfs = manage_cfs
        
        self.project_name = {}
        self.project_name_map = {}
        self.new_app_dir = None
        self.has_tutorial = False
        
        self.max_progress = 10
        self.cur_progress = 0
        self.progress_bar = None
        self.progress_txt = None
        
    def update_progress(self, text, delta):
        """
        This is called by create_project() after the progress window is created.
        """
        self.cur_progress += delta

        self.progress_txt.update(text)
        self.progress_bar.update_bar(self.cur_progress)
        
    def create_project(self, project_name):
        
        github_txt = 'Downloading apps from github\n'
        layout = [[sg.Text(github_txt, key='-PROGRESS_TEXT-', size=(50,2))],
                  [sg.ProgressBar(max_value=self.max_progress, orientation='h', size=(50, 20), key='-PROGRESS-', bar_color=('green', 'white'))]]
        window = sg.Window(f'Create {project_name} Project', layout, keep_on_top=True, finalize=True)

        self.progress_bar = window['-PROGRESS-']
        self.progress_txt = window['-PROGRESS_TEXT-']
        
        # 1. Download github apps
        github_delta = int((self.max_progress/2)/len(self.json.app_list())) # Download gets half of progress bar
        github_apps = GitHubApps(self.git_url, self.usr_app_rel_path)
        github_apps.create_dict()
        for app in self.json.app_list():
            self.update_progress(github_txt, github_delta)
            github_apps.clone(app,quiet_ops=True)
        
        # 2. Add apps to cFS target
        self.update_progress('Adding apps to cFS target\n', 1)
        self.manage_cfs.add_usr_app_list(self.json.app_list())
        sleep(4) # Allow user to see change. 

        # 3. Build cFS target
        build_txt   = 'Building new cFS target\n'
        build_delta = int((self.max_progress/2)-1) # Build gets half progress bar minus 'Add apps to target'
        self.manage_cfs.build_target()
        #TODO: Open loop delay didn't work because sleep interferes with main GUI process window update
        #TODO: Is there a way to get build_target process feedback?
        #for x in range(2, 30, 3):
        #   self.update_progress(build_txt, build_delta)
        #   sleep(4)  
        self.update_progress(build_txt, build_delta)
        self.manage_cfs.restart_main_gui(self.json.popup_instructions())

        window.close()
      
        return True
        
###############################################################################

class CreateProject():
    """
    Create a database of projects and a display for a user to select
    one.  Project titles defined in the JSON files are used as template
    identifiers for screen displays and as dictionary keys 
    """
    def __init__(self, projects_url, projects_path, git_url, usr_app_rel_path, manage_cfs):

        self.projects_url  = projects_url
        self.projects_path = projects_path
        self.manage_cfs = manage_cfs
        
        self.project_template_titles = []
        self.project_template_lookup = {}  # [title]  => ProjectTemplate
        for project_template_folder in os.listdir(self.projects_path):
            logger.debug("Project template folder: " + project_template_folder)
            #todo: ProjectTemplate constructor could raise exception if JSON doesn't exist or is malformed
            project_template_json_file = os.path.join(projects_path, project_template_folder, PROJECT_JSON_FILE)
            if os.path.exists(project_template_json_file):
                project_template = ProjectTemplate(os.path.join(projects_path, project_template_folder),git_url,usr_app_rel_path,self.manage_cfs)
                self.project_template_titles.append(project_template.json.title())
                self.project_template_lookup[project_template.json.title()] = project_template                    
        logger.debug("Project Template Lookup " + str(self.project_template_lookup))
                
        self.window  = None
        self.selected_app = None
        
    def create_window(self):
        """
        The intro text box size must be manually adjusted with the project_template_layout sizes. 
        """
        hdr_label_font = ('Arial bold',14)
        hdr_value_font = ('Arial',12)
        
        project_template_layout = []
        for project_title, project_meta_data in self.project_template_lookup.items():
            logger.debug(f'self.project_template_lookup[{project_title}] => {str(project_meta_data)}')
            project_template_layout.append([sg.Radio(project_title, "PROJECT_TEMPLATES", default=False, font=hdr_value_font, size=(25,0), key=project_title, enable_events=True),
                                            sg.Text(project_meta_data.json.short_description(), font=hdr_value_font, size=(50,1))])
        
        layout = [
                  [sg.Text(f"Create a new cFS project target. This includes downloading libs/apps from github, adding them to the cFS target build files, and building the new target. Project documents are in 'Help->Project Docs...' and videos are at '{self.projects_url}'.\n", font=hdr_value_font, size=(80,5))],
                  [sg.Text('Select Project: ', font=hdr_label_font)],
                  project_template_layout, 
                  [sg.Text('', font=hdr_value_font)],
                  [sg.Text('Project Name: ', font=hdr_label_font), sg.InputText(key='-PROJECT_NAME-', font=hdr_value_font, size=(25,1))],
                  [sg.Text('\n', font=hdr_value_font)],
                  [sg.Button('Create Project', button_color=('SpringGreen4'), pad=(2,0)), sg.Button('Description', pad=(2,0)), sg.Button('Cancel', pad=(2,0))]
                 ]
        
        window = sg.Window('Create Project', layout, resizable=True, finalize=True) # auto_size_text=True, modal=True)
        return window


    def execute(self):
        """
        """        
        self.window = self.create_window() 
        
        while True: # Event Loop
            
            self.event, self.values = self.window.read()

            if self.event in (sg.WIN_CLOSED, 'Cancel') or self.event is None:       
                break
            
            self.selected_project = None
            for title in self.project_template_titles:
                if self.values[title] == True:
                    self.selected_project = self.project_template_lookup[title]
                    break
                    
            if self.event == 'Description':
                if self.selected_project is not None:
                    description = ""
                    for description_line in self.selected_project.json.description():
                        description += description_line + '\n'
                    sg.popup(description, title=self.selected_project.json.title(), font='Courier 12', line_width=sg.MESSAGE_BOX_LINE_WIDTH*2)
                else:
                    sg.popup("Please select a project", title="Create Project", modal=False)
                
            if self.event in self.project_template_titles:
                if self.selected_project is not None:
                    self.window["-PROJECT_NAME-"].update(self.selected_project.json.title())
                
            if self.event == 'Create Project':
                project_name = self.values['-PROJECT_NAME-']
                if len(project_name) > 0:
                    try:
                        self.selected_project.create_project(project_name)
                    except Exception as e:
                        print(e)
                        sg.popup(f'Failed to create {project_name}', title="Create Project", keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
                break
                
        self.window.close()


###############################################################################

if __name__ == '__main__':

    config = configparser.ConfigParser()
    config.read('../basecamp.ini')

    PROJECTS_PATH    = config.get('PATHS','PROJECTS_PATH')
    GIT_URL          = config.get('APP','APP_STORE_URL')
    USR_APP_REL_PATH = config.get('PATHS', 'USR_APP_PATH')
    
    projects_dir = os.path.join(os.getcwd(),'..', PROJECTS_PATH) 
    usr_app_dir  = os.path.join(os.getcwd(),'..', USR_APP_REL_PATH) 
    print (f'projects_dir: {projects_dir}')
    CreateProject(projects_dir,GIT_URL,usr_app_dir).execute()
    
    


