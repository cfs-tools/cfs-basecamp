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
        Provide classes that manage the creation of apps from templates.
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

import logging
logger = logging.getLogger(__name__)

if __name__ == '__main__':
    from jsonfile import JsonFile
    from utils    import compress_abs_path
else:
    from .jsonfile import JsonFile
    from .utils    import compress_abs_path

from tools import PySimpleGUI_License
import PySimpleGUI as sg

TEMPLATE_JSON_FILE = 'app_template.json'
TEMPLATE_VAR_UPPER = "@TEMPLATE@"
TEMPLATE_VAR_MIXED = "@Template@"
TEMPLATE_VAR_LOWER = "@template@"

TEMPLATE_COMMENT_START = "/*##"
TEMPLATE_COMMENT_END   = "##*/"

###############################################################################

class AppTemplateJson(JsonFile):
    """
    Manage app template JSON files.
    """
    def __init__(self, json_file):
        super().__init__(json_file)
        
    def version(self):
        return self.json['version']

    def reset_child(self):
        pass
        
    def app_name(self):
        return self.json['default-app-name']
        
    def directories(self):
        return self.json['dir']

    def subdir_path(self, dir_list):
        return dir_list['path']

    def subdir_files(self, dir_list):
        return dir_list['files']
          

###############################################################################

class AppTemplate():
    """
    """
    def __init__(self, template_path):

        self.path = template_path
        self.json = AppTemplateJson(os.path.join(template_path, TEMPLATE_JSON_FILE))
        self.dirs = self.json.directories()
        
        self.app_name = {}
        self.app_name_map = {}
        self.new_app_dir = None
        self.has_tutorial = False
        
    def create_app(self, app_name, cfs_app_dir):

        app_created = False
        
        self.app_name = {'UPPER': app_name.upper(), 'LOWER': app_name.lower(), 'MIXED': app_name.capitalize()}
        self.app_name_map = {
                               TEMPLATE_VAR_UPPER: self.app_name['UPPER'],
                               TEMPLATE_VAR_LOWER: self.app_name['LOWER'],
                               TEMPLATE_VAR_MIXED: self.app_name['MIXED'],
                            }
        
        self.new_app_dir = compress_abs_path(os.path.join(cfs_app_dir, self.app_name['LOWER']))
        
        make_dir = True
        if (os.path.exists(self.new_app_dir)):
        
            event, values = sg.Window('Warning',
                  [[sg.T('Destination directory %s already exists.\nDo you want to overwrite it?' % self.new_app_dir)],
                  [sg.B('Yes'), sg.B('No') ]]).read(close=True)
            if event == 'No':
                make_dir = False
                print('Create app aborted')
        if make_dir:
            try: 
                os.makedirs(self.new_app_dir) 
            except OSError as e:
                if e.errno != errno.EEXIST:
                    print("Error creating new app directory" + self.new_app_dir)
                    raise  # raises the error again
              
            for dir in self.dirs:
            
                subdir_path  = self.json.subdir_path(dir)
                subdir_files = self.json.subdir_files(dir)
                logger.debug("path = " + subdir_path)
                logger.debug("files = " + str(subdir_files))
                
                if subdir_path == "tutorial":
                    self.has_tutorial = True
                
                if len(subdir_path) > 0:
                    template_file_path = os.path.join(self.path, subdir_path)
                    new_app_file_path  = os.path.join(self.new_app_dir, subdir_path)
                else:
                    template_file_path = self.path
                    new_app_file_path  = self.new_app_dir
            
                try: 
                    os.makedirs(new_app_file_path) 
                except OSError as e:
                    if e.errno != errno.EEXIST:
                        print("Error creating new app subdirectory" + new_app_file_path)
                        raise  # raises the error again
               
                try:
                    for template_file in subdir_files:
                        self.instantiate_file(template_file_path, new_app_file_path, template_file)
                except Exception as e:
                    sg.popup("Exception:\n"+str(e), title="Create Application Error", modal=False)
                    raise  # raises the error again
                    
            app_created = True
      
        return (app_created, self.new_app_dir)
        
    def instantiate_file(self, template_file_path, new_app_file_path, template_file):
        """
        Since template files should be short no need for fancy optimized 
        algorithms. Keep it simple and readable. 
        Comment blocks delimited with keywords are skipped.
        Binary files have an ampersand(&) appended to the filename. This indicates that the
        file should be copied and not interpreted as a text template file. 
        """
        logger.debug("template_file_path = " + template_file_path)
        logger.debug("new_app_file_path  = " + new_app_file_path)
        logger.debug("template_file      = " + template_file)
        replace_tokens = True
        instantiated_text = ""
      
        if template_file.endswith('&'):
            binary_file = template_file[:-1]
            src_pathfile = os.path.join(template_file_path, binary_file)
            dst_pathfile = os.path.join(new_app_file_path, binary_file)
            print(f'Copying binary file from {src_pathfile} to {dst_pathfile}')
            try:
                shutil.copyfile(src_pathfile, dst_pathfile)
            except FileNotFoundError:
                logger.error(f'Error copying binary file from {src_pathfile} to {dst_pathfile}')
        else:
            with open(os.path.join(template_file_path, template_file)) as f:
                for line in f:

                    # Continue skipping until end of comment. Assume nothing else on last comment line
                    if (replace_tokens):
                        if TEMPLATE_COMMENT_START in line:
                            replace_tokens = False
                        else:
                           # Replace all occurrences for each case
                           for template_token, app_name in self.app_name_map.items():
                               line = line.replace(template_token, app_name)
                           instantiated_text += line
                    else:
                        replace_tokens = True if TEMPLATE_COMMENT_END in line else False

            # Replace template variable in filename 
            for template_token, app_name in self.app_name_map.items():
                template_file = template_file.replace(template_token, app_name)
            
            with open(os.path.join(new_app_file_path, template_file), 'w') as f:
                f.write(instantiated_text)
    			

        
###############################################################################

class CreateApp():
    """
    Create a database of app_templates and a display for a user to select
    one.  App_template titles defined in the JSON files are used as template
    identifiers for screen displays and as dictionary keys 
    """
    def __init__(self, app_templates_path, usr_app_path):

        self.usr_app_path = usr_app_path
        self.app_templates_path = app_templates_path
        self.app_template_titles = []
        self.app_template_lookup = {}  # [title]  => AppTemplate
        for app_template_folder in os.listdir(self.app_templates_path):
            logger.debug("App template folder: " + app_template_folder)
            #todo: AppTemplate constructor could raise exception if JSON doesn't exist or is malformed
            app_template_json_file = os.path.join(app_templates_path, app_template_folder, TEMPLATE_JSON_FILE)
            if os.path.exists(app_template_json_file):
                app_template = AppTemplate(os.path.join(app_templates_path, app_template_folder))
                self.app_template_titles.append(app_template.json.title())
                self.app_template_lookup[app_template.json.title()] = app_template
                    
        logger.debug("App Template Lookup " + str(self.app_template_lookup))
                
        self.window  = None
        self.selected_app = None
        
    def create_window(self):
        """
        The intro text box size must be manually adjusted with the app_template_layout sizes. 
        """
        hdr_label_font = ('Arial bold',14)
        hdr_value_font = ('Arial',12)
        
        app_template_layout = []
        for app_title, app_meta_data in self.app_template_lookup.items():
            logger.debug(f'self.app_template_lookup[{app_title}] => {str(app_meta_data)}')
            app_template_layout.append([sg.Radio(app_title, "APP_TEMPLATES", default=False, font=hdr_value_font, size=(12,0), key=app_title, enable_events=True),
                                        sg.Text(app_meta_data.json.short_description(), font=hdr_value_font, size=(65,1))])
        
        layout = [
                  [sg.Text("\nAfter you create an app using a template, you must add it to the cFS target using 'File->Add Target App'. Each app includes a coding tutorial that is added to the 'Tutorials' menu.\n", font=hdr_value_font, size=(80,5))],        
                  [sg.Text('Select Application Template: ', font=hdr_label_font)],
                  app_template_layout, 
                  [sg.Text('', font=hdr_value_font)],
                  [sg.T('Application Name: ', font=hdr_label_font), sg.In(key='-APP_NAME-', font=hdr_value_font, size=(25,1))],
                  [sg.Text('', font=hdr_value_font)],
                  [sg.Button('Create App', button_color=('SpringGreen4'), pad=(2,0)), sg.Button('Description', pad=(2,0)), sg.Button('Cancel', pad=(2,0))]
                 ]
        
        window = sg.Window('Create User App', layout, modal=False)
        return window


    def execute(self):
        """
        """        
        self.window = self.create_window() 
        
        while True: # Event Loop
            
            self.event, self.values = self.window.read()

            if self.event in (sg.WIN_CLOSED, 'Cancel') or self.event is None:       
                break
            
            self.selected_app = None
            for title in self.app_template_titles:
                if self.values[title] == True:
                    self.selected_app = self.app_template_lookup[title]
                    break
                    
            if self.event == 'Description':
                if self.selected_app is not None:
                    description = ""
                    for description_line in self.selected_app.json.description():
                        description += description_line + '\n'
                    sg.popup(description, title=self.selected_app.json.title(), font='Courier 12', line_width=sg.MESSAGE_BOX_LINE_WIDTH*2)
                else:
                    sg.popup("Please select an application template", title="Create Application", modal=False)
                
            if self.event in self.app_template_titles:
                if self.selected_app is not None:
                    self.window["-APP_NAME-"].update(self.selected_app.json.app_name())
                
            if self.event == 'Create App':
                app_name = self.values['-APP_NAME-']
                new_app_dir = '[directory not defined]'
                if len(app_name) > 0:
                    try:
                        app_created, new_app_dir = self.selected_app.create_app(app_name, os.path.join(os.getcwd(),self.usr_app_path))
                        if app_created:
                            status_text = f'Successfully created {app_name} in {new_app_dir}\n'
                            instruct_text = f"> Use 'Develop->Add App' to add {app_name} to the cFS target.\n"
                            tutorial_text = ''
                            if self.selected_app.has_tutorial:
                                tutorial_text = "> A coding tutorial will be added to the 'Learn' dropdown menu when Basecanp is restarted.\n"
                            popup_text = f'{status_text}{instruct_text}{tutorial_text}\n'
                            sg.popup(popup_text, line_width=100, title="Create Application", modal=False)
                            break                
                    except:
                        sg.popup(f'Failed to create {app_name} in {new_app_dir}', title="Create Application", modal=False)
                            
        self.window.close()


###############################################################################

if __name__ == '__main__':

    config = configparser.ConfigParser()
    config.read('../basecamp.ini')

    APP_TEMPLATES_PATH = config.get('PATHS','APP_TEMPLATES_PATH')
    USR_APP_PATH  = config.get('PATHS','USR_APP_PATH')
    
    templates_dir = os.path.join(os.getcwd(),'..', APP_TEMPLATES_PATH) 
    usr_app_dir   = os.path.join(os.getcwd(),'..', USR_APP_PATH) 
    print (f'Directories: templates_dir: {templates_dir}, usr_app_dir: {usr_app_dir}')
    CreateApp(templates_dir, usr_app_dir).execute()
    
    #Test without GUI
    #app_template = AppTemplate(template_dir)
    #app_template.create_app('hello', os.path.join(os.getcwd(),'../../../cfs-apps'))

    


