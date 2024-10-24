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

    Purpose:
        Provide classes that manage downloading and installing apps from git repos
        
    Notes:    
        Assumes the exact same app name is used for
        - App directory
        - App Electronic Data Sheet (EDS) file
        - App cFS spec JSON file 
"""

import sys
import time
import os
import requests
import json
import configparser
import shutil
from datetime import datetime

import logging
logger = logging.getLogger(__name__)

if __name__ == '__main__':
    from eds   import AppEds
    from utils import compress_abs_path
else:
    from .eds   import AppEds
    from .utils import compress_abs_path

from tools import PySimpleGUI_License
import PySimpleGUI as sg


###############################################################################

class GitHubAppProject():
    '''
    Manage the interface to cFS apps in github repos  
    '''
    def __init__(self, git_url, usr_app_rel_path):
        """
        usr_app_rel_path  - Relative path to where git repos should be cloned into
        """
        self.usr_clone_path = usr_app_rel_path
        self.git_url  = git_url
        self.app_repo = None
        self.app_dict = {}
         
         
    def create_dict(self):
        """
        Queries git URL to a list of apps. A dictionary is created using app
        names as the keys. This function is not part of the constructor 
        to allow the caller to interact with the user in the event that the
        URL can't be accessed.
        """
        ret_status = False
        try:
            self.app_repo = requests.get(self.git_url)
            if self.app_repo.status_code == 200:
                app_repo_list = self.app_repo.json()
                # Create a dictionary with app names as the key
                for repo in app_repo_list:
                    self.app_dict[repo['name']] = repo
                ret_status = True
        except requests.exceptions.ConnectionError as e:
            pass
            
        return ret_status
        

    def clone(self, app_name):
        """
        """
        if app_name in self.app_dict:
            clone_repo = True
            target_dir = compress_abs_path(os.path.join(os.getcwd(), self.usr_clone_path, app_name))
            if os.path.exists(target_dir):
                overwrite = sg.popup_yes_no(f"{target_dir} exists. Do you want to overwrite it?",  title="AppStore")
                if (overwrite == 'Yes'):
                    shutil.rmtree(target_dir)
                else:
                    clone_repo = False
            if clone_repo:
                saved_cwd = os.getcwd()
                os.chdir(self.usr_clone_path)
                clone_url = self.app_dict[app_name]["clone_url"]
                sys_status = os.system("git clone {}".format(self.app_dict[app_name]["clone_url"]))
                if (sys_status == 0):
                    sg.popup(f'Successfully cloned {app_name} into {target_dir}', title='AppStore')
                else:
                    sg.popup(f'Error cloning {app_name} into {target_dir}', title='AppStore Error')
                os.chdir(saved_cwd)
     
    def get_descr(self, app_name):
        """
        """
        descr = ''
        if app_name in self.app_dict:
            descr = self.app_dict[app_name]['description']
        return descr

    def get_topics(self, app_name):
        """
        """
        topics = []
        if app_name in self.app_dict:
            topics = self.app_dict[app_name]['topics']
        return topics


###############################################################################

class AppSpec():
    """
    The access methods are defined according to the activities a developer
    needs to do to integrate an app.
    This design supports topic IDs being defined in multiple EDS files per
    lib/app.
    Only one JSON spec per lib/app is allowed and using the <app name>.json
    naming convention.    
    """
    def __init__(self, app_path, app_name):

        self.app_path   = app_path
        self.app_name   = app_name
        self.eds_path   = os.path.join(app_path, 'eds')
        self.json_file  = os.path.join(app_path, app_name+'.json')
        self.eds_specs  = []
        self.cmd_topics = []
        self.tlm_topics = []
        self.is_valid   = False
        self.has_topics = False
        self.json = None
        self.app  = None
        self.cfs  = None

        # 'is_valid' and 'has_topics' are False and will be set to True as needed
        # EDS files are required for apps and optional for libraries to be valid
        if self.read_json_file():
            self.read_eds_files()

    def read_eds_files(self):
        """
        JSON spec must be loaded prior to calling this function 
        """
        # Libraries don't require an EDS spec
        if self.cfs['cfe-type'] == 'CFE_LIB':
            self.is_valid = True

        if os.path.exists(self.eds_path):
            eds_dir   = os.listdir(self.eds_path)
            eds_files = [filename for filename in eds_dir if '.xml' in filename]
            print(f'*** eds_files: {eds_files}')
            for eds_filename in eds_files:
                eds_file = os.path.join(self.eds_path, eds_filename)
                eds_spec = self.read_eds_file(eds_file)
                if eds_spec is not None:
                    self.eds_specs.append(eds_spec)
            if len(self.eds_specs) > 0:
                for eds_spec in self.eds_specs:
                    self.cmd_topics += eds_spec.cmd_topics()
                    self.tlm_topics += eds_spec.tlm_topics()
                print(f'*** self.cmd_topics: {self.cmd_topics}')
                print(f'*** self.tlm_topics: {self.tlm_topics}')
                if (len(self.cmd_topics) > 0 or len(self.tlm_topics) > 0):
                    self.has_topics = True
                    if self.cfs['cfe-type'] == 'CFE_APP':
                        self.is_valid = True
        else:
            if self.cfs['cfe-type'] == 'CFE_APP':
                sg.popup(f'App is missing an EDS spec. Expected {self.eds_path} to exist', title='AppStore Error', grab_anywhere=True, modal=False)
        
            
    def read_json_file(self):
    
        if os.path.exists(self.json_file):
            try:
                f = open(self.json_file)
                self.json = json.load(f)
                f.close()
            except:
                sg.popup(f'Error loading JSON spec file {self.json_file}', title='AppStore Error', grab_anywhere=True, modal=False)
                return False
        else:
            sg.popup(f'Error loading JSON spec file {self.json_file}', title='AppStore Error', grab_anywhere=True, modal=False)
            return False
        
        try:
            self.app = self.json['app']
            try:
                self.cfs = self.app['cfs']
            except:
                sg.popup(f"The JSON spec file {self.json_file} does not contain the required 'cfs' object", title='AppStore Error', grab_anywhere=True, modal=False)
                return False
        except:
            sg.popup(f"The JSON spec file {self.json_file} does not contain the required 'app' object", title='AppStore Error', grab_anywhere=True, modal=False)
            return False
        
        return True
        
    def has_topic_ids(self):
        return self.has_topics
        
    def read_eds_file(self, eds_filename):        
        eds_obj = None
        try:
            eds_obj = AppEds(eds_filename)
        except Exception as e: 
            if (self.cfs['cfe-type'] == 'CFE_APP'):
                sg.popup(f'Exception {repr(e)} raised when attempting to read app EDS file {eds_filename}', title='AppStore Error', grab_anywhere=True, modal=False)
            elif (self.cfs['cfe-type'] == 'CFE_LIB'):
                sg.popup(f'Exception {repr(e)} raised when attempting to read library EDS file {eds_filename}', title='AppStore Error', grab_anywhere=True, modal=False)
            else:
                pass
        return eds_obj        
       
    def get_app_info(self):
        info = {}
        info['title']       = self.app['title']
        info['version']     = self.app['version']
        info['supplier']    = self.app['supplier']
        info['url']         = self.app['url']
        info['description'] = self.app['description']
        info['requires']    = self.app['requires']
        return info

    def get_cmd_topics(self):
        return self.cmd_topics
    
    def get_tlm_topics(self):
        return self.tlm_topics
    
    def get_targets_cmake_files(self):
        """
        The targets.cmake file needs
           1. The app's object file name for the 'cpu1_APPLIST'
           2. The names of all the tables that need to be copied from the app's tables directory into
              the cFS '_defs' directory 
        """
        files = {}
        files['obj-file'] = self.cfs['obj-file']
        files['tables']   = self.cfs['tables']
        return files

    def get_startup_scr_entry(self):
        '''
        Create an cfe_es_startup.scr entry string that contains the following fields:
        
        1. Object Type      -- CFE_APP for an Application, or CFE_LIB for a library.
        2. Filename         -- This is a cFE Virtual filename, not a vxWorks device/pathname
        3. Entry Point      -- This is the "main" function for Apps.
        4. CFE Name         -- The cFE name for the APP or Library
        5. Priority         -- This is the Priority of the App, not used for Library
        6. Stack Size       -- This is the Stack size for the App, not used for the Library
        7. Load Address     -- This is the Optional Load Address for the App or Library. Currently not implemented
                               so keep it at 0x0.
        8. Exception Action -- This is the Action the cFE should take if the App has an exception.
                               0        = Just restart the Application
                               Non-Zero = Do a cFE Processor Reset

        CFE_APP, file_xfer,       FILE_XFER_AppMain,   FILE_XFER,    80,   16384, 0x0, 0;
        '''
        entry_str = ''
        try:
            entry_str = self.cfs['cfe-type']      + ', ' + \
                        self.cfs['obj-file']      + ', ' + \
                        self.cfs['entry-symbol']  + ', ' + \
                        self.cfs['name']          + ', ' + \
                        str(self.cfs['priority']) + ', ' + \
                        str(self.cfs['stack'])    + ', 0x0, ' + \
                        str(self.cfs['exception-action']) + ';' 
        except:
            sg.popup(f'Error creating targets.cmake entry due to missing or malformed JSON file.\nPartial entry string = {self.json_file}', title='AppStore Error', grab_anywhere=True, modal=False)
        
        return entry_str


###############################################################################

class ManageUsrApps():
    """
    Discover what user apps exists (each app in separate directory) and
    create a 'database' of app specs that can be used by the user to integrate
    apps into a cFS target.
    """
    def __init__(self, usr_app_abs_path):

        self.path = usr_app_abs_path
        self.app_specs = {}
        
        usr_app_list = os.listdir(usr_app_abs_path)
        usr_app_list.sort()
        # Assumes app directory name equals app name
        for app_name in usr_app_list:
            app_path = os.path.join(usr_app_abs_path, app_name)
            if os.path.isdir(os.path.join(usr_app_abs_path, app_name)):
                # AppSpec manages exceptions so caller can simply check 'is_valid'
                app_spec = AppSpec(app_path, app_name)
                if app_spec.is_valid:
                    self.app_specs[app_name] = app_spec        
        
    def get_app_specs(self):
        return self.app_specs

    def get_app_spec(self, app_name):
        return self.app_specs[app_name]
            
              
###############################################################################

class AppStore():
    """
    Manage the user interface for downloading apps from github and cloning
    them into the user's app directory. 
    """
    def __init__(self, git_url, usr_app_rel_path, git_topic_include, git_topic_exclude):
        """
        git_topic_include - List of github topics identifying repos to be included
        git_topic_exclude - List of github topics identifying repos to be excluded
        """
        self.git_topic_include = git_topic_include
        self.git_topic_exclude = git_topic_exclude 
        self.usr_app_abs_path = compress_abs_path(os.path.join(os.getcwd(), usr_app_rel_path))
        self.git_app_repo = GitHubAppProject(git_url, usr_app_rel_path)
        self.git_app_repo_keys = [] # keys of app repos that pass the include/exclude filters 
        self.window  = None

        
    def create_window(self):
        """
        """
        hdr_label_font = ('Arial bold',14)
        hdr_value_font = ('Arial',12)
        window_width   = 100
        app_layout = []
        for app in self.git_app_repo.app_dict.keys():
            topics = self.git_app_repo.get_topics(app)
            if any(x in topics for x in self.git_topic_include) and not any(x in topics for x in self.git_topic_exclude):
                self.git_app_repo_keys.append(app)
                app_layout.append([sg.Checkbox(app.upper(), default=False, font=hdr_label_font, size=(10,0), key=f'-{app}-'),  
                                  sg.Text(self.git_app_repo.get_descr(app), font=hdr_value_font, size=(window_width,1))])
                
        layout = [
                  [sg.Text("Select one or more apps to download and click the <Download> button to add them to the usr/app repositories:", font=hdr_label_font, size=(window_width,None))],
                  [sg.Text("   - Follow the steps in 'File->Add User App to Target' to add the apps/libs to add the cFS target", font=hdr_value_font)],
                  [sg.Text("   - The Hello World tutorial in 'Tutorials->Create App Tool' describes the steps to add an app to a cFS target", font=hdr_value_font)],
                  [sg.Text("   - An app's JSON spec file has a 'requires' parameter that identifies dependencies that must be installed prior to the app\n", font=hdr_value_font)],
                  [app_layout],
                  [sg.Text("")],
                  [sg.Button('Download', font=hdr_label_font, button_color=('SpringGreen4'), pad=(2,0)), sg.Button('Cancel', font=hdr_label_font, pad=(2,0))]
                 ]

        window = sg.Window('Download User App', layout, modal=False)
        return window


    def gui(self):
        """
        """        
        self.window = self.create_window() 
        
        while True: # Event Loop
            
            self.event, self.values = self.window.read()

            if self.event in (sg.WIN_CLOSED, 'Cancel') or self.event is None:       
                break
            
            if self.event == 'Download':
                print(f'self.git_app_repo_keys={self.git_app_repo_keys}')
                for app in self.git_app_repo_keys:
                    if self.values[f'-{app}-'] == True:
                        self.git_app_repo.clone(app) # Clone reports status to user via popups
                break
                
        self.window.close()

    def execute(self):
        """
        """        
        if self.git_app_repo.create_dict():
            self.gui()
        else:
            sg.popup(f"Error accessing the git url\n   '{self.git_app_repo.git_url}'\n\nVerify your network connection and the basecamp.ini APP_STORE_URL definition.\n", title='AppStore Error')


###############################################################################

if __name__ == '__main__':

    config = configparser.ConfigParser()
    config.read('../basecamp.ini')

    git_url = config.get('APP','APP_STORE_URL')
    usr_app_abs_path = compress_abs_path(os.path.join(os.getcwd(),'..', config.get('PATHS', 'USR_APP_PATH'))) 

    #app_store = AppStore(git_url, usr_app_path)
    #app_store.execute()
    
    
    manage_usr_apps = ManageUsrApps(usr_app_abs_path)
    
    berry_imu = manage_usr_apps.get_app_spec('berry_imu')
    print(berry_imu.get_targets_cmake_files())
    print(berry_imu.get_startup_scr_entry())
    
    gpio_demo = manage_usr_apps.get_app_spec('gpio_demo')
    print(gpio_demo.get_targets_cmake_files())
    print(gpio_demo.get_startup_scr_entry())
    
    

