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
        projects automate the process of downloading apps and building a 
        new cFS target to implement a project cFS target.        
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
    from utils      import AppStoreDef, compress_abs_path
    from appstore   import GitHubAppRepo
    from cfstarget  import ManageCfs
else:
    from .jsonfile  import JsonFile
    from .utils     import AppStoreDef, compress_abs_path
    from .appstore  import GitHubAppRepo
    from .cfstarget import ManageCfs

from tools import PySimpleGUI_License
import PySimpleGUI as sg

PROJECT_JSON_FILE = 'project.json'

###############################################################################

class ProjectTemplateJson(JsonFile):
    """
    Provide a functional interface to project template JSON files. Ideally all
    JSON objects are present. However, only app-list is required so trying to
    access the other objects may generate an exception.
    """
    def __init__(self, json_file):
        super().__init__(json_file)
        self.support_files = None
        self.support_files_valid = False # True means self.support_files len greater than zero
        
    def version(self):
        return self.json['version']
    
    def released(self):
        """
        A released project will be displayed in available projects window. The criteria
        for determining whether a project is released may change so it is encapsulated
        in this function.         
        """
        released = True
        if "alpha" in self.json['release']:
            released = False
        return released
        
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

    def load_support_files(self):
        self.support_files = {}
        try:
           self.support_files = self.json['support-files']
           if len(self.support_files) > 0:
               self.support_files_valid = True
        except:
           pass
        return  self.support_files

    def get_support_file(self, support_file_key):
        """
        All support file objects use the same syntax of a string of comma
        separated filenames 
        """
        support_file_list = []
        if self.support_files is None:
            self.load_support_files() 

        if self.support_files_valid:
            try:
                support_file_string = self.support_files[support_file_key]
                # Check greater than 0 length in case a couple of spaces
                if len(support_file_string) > 3:
                    support_file_list = support_file_string.split(',')
            except:
                pass
        
        return support_file_list


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
        
        github_delta = int((self.max_progress/2-1)/len(self.json.app_list())) # Download gets half of progress bar minus 'Add apps to target' step
        build_range  = int((self.max_progress/2))     
        build_delta  = 1
        # Build gets half progress bar 
        # 1. Download github apps
        git_app_repo = GitHubAppRepo(self.git_url, AppStoreDef.BASECAMP_REPO_BRANCH, self.usr_app_rel_path, quiet_ops=True)
        git_app_repo.create_dict()
        for app in self.json.app_list():
            self.update_progress(github_txt, github_delta)
            # Clone functions report status to user via popups
            if app.startswith(AppStoreDef.PROXY_APP_PREFIX):
                ret_status = git_app_repo.clone_proxy_repo(app)
            else:
                ret_status = git_app_repo.clone_basecamp_repo(app) 
        
        # 2. Copy support files        
        copied_support_files = False
        try:
            self.update_progress('Copying support files to gnd-sys directories\n', 1)
            self.copy_support_files()
            sleep(4) # Allow user to see change 
            copied_support_files = True
        except Exception as e:
            sg.popup(f'Failed to copy {project_name} support files.\nException: {e}\nWill attempt to create cFS target next.', title="Create Project Error", keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
        
        # 3. Add apps to cFS target
        added_apps_to_target = False
        try:
            self.update_progress('Adding apps to cFS target\n', 1)
            # Convert any proxy names to their real counterpart name by removing proxy prefix
            app_list = [app_name.replace(AppStoreDef.PROXY_APP_PREFIX, '') for app_name in self.json.app_list()]
            self.manage_cfs.add_usr_app_list(app_list)
            sleep(4) # Allow user to see change 
            added_apps_to_target = True
        except Exception as e:
            sg.popup(f'Failed to create {project_name}.\nException: {e}', title="Create Project Error", keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)

        # 4. Build cFS target
        """
        This is a very crude display management scheme. The cFS build process runs asynchronously,
        doesn't provide feedback on progress and the time to build the target is unknown. Therefore
        the goal of this algorithm is to let the user know progress is being made and end with a
        dialogue that tells them how they know the build is done. 
        
        Can't use sleep() because it interferes with main GUI process window update so using window.read()
       
        """
        if added_apps_to_target:
            build_txt    = 'Building new cFS target\n'
            delay_factor = build_delta * 4000  # Milliseconds for each delta
            self.manage_cfs.build_target()     # Start async build process
            for i in range(1, build_range):
                self.update_progress(build_txt, build_delta)
                window.read(timeout=delay_factor)
        
        window.close()
        self.manage_cfs.restart_main_gui(self.json.popup_instructions())
      
        return True
 
    def copy_support_files(self):
        """
        Assumes all support files are in the project's base directory. 
        """
        
        gnd_sys_app_path = compress_abs_path(os.path.join(self.path, '../../app'))
        print(f'gnd_sys_app_path: {gnd_sys_app_path}')
        config = configparser.ConfigParser()
        config.read(os.path.join(gnd_sys_app_path,'basecamp.ini'))

        support_dict = {
            'cmd-sender':  'CMD_SENDER_PATH',
            'file-server': 'FLT_SERVER_PATH',
            'scripts':     'SCRIPT_PATH',
            'templates':   'APP_TEMPLATES_PATH',
            'tutorials':   'TUTORIALS_PATH'
            }

        for support_file in support_dict:
            file_list = self.json.get_support_file(support_file)
            if file_list != None:
                config_param = support_dict[support_file]
                rel_gnd_path = config.get('PATHS',config_param)           
                for file in file_list:
                    src_pathfile = os.path.join(self.path, file)
                    dst_pathfile = os.path.join(gnd_sys_app_path, rel_gnd_path, file)
                    os.makedirs(os.path.dirname(dst_pathfile), exist_ok=True)
                    copy_file = True
                    if os.path.exists(dst_pathfile):
                        overwrite = sg.popup_yes_no(f'{dst_pathfile} exists.\nDo you want to overwrite it?',  title='Create Project')
                        copy_file = (overwrite == 'Yes')
                    if copy_file:
                        shutil.copyfile(src_pathfile, dst_pathfile)


###############################################################################

class CreateProject():
    """
    Create a database of projects and a display for a user to select
    one. Project titles defined in the JSON files are used as template
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
                project_template = ProjectTemplate(os.path.join(projects_path, project_template_folder), git_url, usr_app_rel_path, self.manage_cfs)
                self.project_template_titles.append(project_template.json.title())
                self.project_template_lookup[project_template.json.title()] = project_template                    
        logger.debug("Project Template Lookup " + str(self.project_template_lookup))
                
        self.window  = None
        self.selected_project = None
        
    def create_window(self):
        """
        The intro text box size must be manually adjusted with the project_template_layout sizes. 
        """
        hdr_label_font = ('Arial bold',14)
        hdr_value_font = ('Arial',12)
        
        project_template_layout = []
        for project_title, project_meta_data in self.project_template_lookup.items():
            logger.debug(f'self.project_template_lookup[{project_title}] => {str(project_meta_data)}')
            if project_meta_data.json.released():
                project_template_layout.append([sg.Radio(project_title, "PROJECT_TEMPLATES", default=False, font=hdr_value_font, size=(25,0), key=project_title, enable_events=True),
                                                sg.Text(project_meta_data.json.short_description(), font=hdr_value_font, size=(50,1))])
        
        layout = [
                  [sg.Text(f'Create a new cFS project target by selecting the project and clicking the <Create Project> button:', font=hdr_label_font, size=(80,None))],
                  [sg.Text(f"   - The <Description> button summarizes the project's objectives", font=hdr_value_font)],
                  [sg.Text(f"   - Project documents are in 'Help->Project Docs...'", font=hdr_value_font)],
                  [sg.Text(f"   - Step-by-step instructions with videos are at '{self.projects_url}'", font=hdr_value_font)],
                  [sg.Text('\nSelect Project: ', font=hdr_label_font)],
                  project_template_layout, 
                  [sg.Text(f'\nProject creation includes downloading libs/apps from github, adding them to the cFS target build files, and building the new target.\n', font=hdr_value_font, size=(80,None))],
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
                                
            if self.event in ['Description','Create Project']:
                if self.selected_project is not None:
                    if self.event == 'Create Project':
                        project_name = self.selected_project.json.title()
                        if len(project_name) > 0:
                            try:
                                self.selected_project.create_project(project_name)
                            except Exception as e:
                                sg.popup(f'Failed to create {project_name}.\nException: {e}', title="Create Project Error", keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
                        break
                    else:
                        description = ""
                        for description_line in self.selected_project.json.description():
                            description += description_line + '\n'
                        sg.popup(description, title=self.selected_project.json.title(), font='Courier 12', line_width=sg.MESSAGE_BOX_LINE_WIDTH*2)
                else:
                    sg.popup("Please select a project", title="Create Project", modal=False)
                
            if self.event in self.project_template_titles:
                self.selected_project = self.project_template_lookup[self.event]
                
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
    
    


