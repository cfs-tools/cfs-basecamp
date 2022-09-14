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
from datetime import datetime

import logging
logger = logging.getLogger(__name__)

if __name__ == '__main__':
    from eds   import AppEds
    from utils import compress_abs_path
else:
    from .eds   import AppEds
    from .utils import compress_abs_path

import PySimpleGUI as sg


###############################################################################

class GitHubAppProject():
    '''
    Manage the interface to cFS apps in github repos  
    '''
    def __init__(self, git_url, usr_app_path, repo_exclusions):
        """
        usr_app_path is an absolute path to where git repos should be cloned into
        repo_exclusions is a list of repos that should not be used
        """
        self.usr_clone_path = usr_app_path
        self.git_url  = git_url
        self.repo_exclusions = repo_exclusions
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
                    print(repo['name'])
                    print(repo['git_url'])
                    if repo['name'] not in self.repo_exclusions:
                        self.app_dict[repo['name']] = repo
                #print(self.app_dict['kit_ci'])
                ret_status = True
        except requests.exceptions.ConnectionError as e:
            pass
            #print (e)
            
        return ret_status
        

    def clone(self, app_name):
        """
        """
        ret_status = False
        if app_name in self.app_dict:
            saved_cwd = os.getcwd()
            os.chdir(self. usr_clone_path)
            clone_url = self.app_dict[app_name]["clone_url"]
            print("Cloning " + clone_url)
            os.system("git clone {}".format(self.app_dict[app_name]["clone_url"]))
            os.chdir(saved_cwd)
            ret_status = True
        return ret_status
        
      
    def get_descr(self, app_name):
        """
        """
        descr = ''
        if app_name in self.app_dict:
            descr = self.app_dict[app_name]['description']
        return descr


###############################################################################

class AppSpec():
    '''
    The access methods are defined according to the activities a developer
    needs to do to integrate an app.
    '''
    def __init__(self, app_path, app_name):

        self.app_path  = app_path
        self.app_name  = app_name
        self.eds_file  = os.path.join(app_path, 'eds', app_name+'.xml')
        self.json_file = os.path.join(app_path, app_name+'.json')
        print(self.eds_file)
        print(self.json_file)
        self.eds   = None                
        self.valid = False
        self.json  = None
        self.cfs   = None
        if self.read_json_file():
            if self.read_eds_file():
                self.valid = True

    def read_json_file(self):
    
        if os.path.exists(self.json_file):
            try:
                f = open(self.json_file)
                self.json = json.load(f)
                f.close()
                #todo print(str(self.json))
            except:
                sg.popup(f'Error loading JSON spec file {self.json_file}', title='AppStore Error', grab_anywhere=True, modal=False)
                return False
        else:
            sg.popup(f'Error loading JSON spec file {self.json_file}', title='AppStore Error', grab_anywhere=True, modal=False)
            return False
        
        try:
            self.cfs = self.json['app']['cfs']
        except:
            sg.popup(f"The JSON spec file {self.json_file} does not contain the required 'cfs' object", title='AppStore Error', grab_anywhere=True, modal=False)
            return False
        
        #todo print('self.cfs = ' + str(self.cfs))
        return True
        
    def read_eds_file(self):        
        try:
            self.eds = AppEds(self.eds_file)
        except:
            sg.popup(f'Error parsing EDS file {self.eds_file}', title='AppStore Error', grab_anywhere=True, modal=False)
            return False
        return True        
        
    def get_cmd_topics(self):
        return self.eds.cmd_topics()
    
    def get_tlm_topics(self):
        return self.eds.tlm_topics()
    
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
    apps into their cFS target.
    """
    def __init__(self, usr_app_path):

        self.path = usr_app_path
        self.app_specs = {}
        
        usr_app_list = os.listdir(usr_app_path)
        usr_app_list.sort()
        # Assumes app directory name equals app name
        for app_name in usr_app_list:
            print("User app folder/name: " + app_name)
            #todo: AppSpec constructor could raise exception if JSON doesn't exist or is malformed
            app_path = os.path.join(usr_app_path, app_name)
            if os.path.isdir(os.path.join(usr_app_path, app_name)):
                app_spec = AppSpec(app_path, app_name)
                if app_spec.valid:
                    self.app_specs[app_name] = app_spec        
        
        print("User app specs: " + str(self.app_specs))

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
    def __init__(self, git_url, usr_app_path, repo_exclusions):

        self.usr_app_path = usr_app_path
        self.git_app_repo = GitHubAppProject(git_url, usr_app_path, repo_exclusions)
        self.window  = None

        
    def create_window(self):
        """
        """
        hdr_label_font = ('Arial bold',12)
        hdr_value_font = ('Arial',12)
        app_layout = []
        for app in self.git_app_repo.app_dict.keys():
            print(app)
            app_layout.append([sg.Radio(app, "APPS", default=False, font=hdr_label_font, size=(10,0), key='-%s-'%app),  
                               sg.Text(self.git_app_repo.get_descr(app), font=hdr_value_font, size=(100,1))])
                
        layout = [
                  [sg.Text("Select an app to download then follow the steps in 'Add App to cFS'. See 'Add App' tutorial if you are unfamiliar with the steps.\n", font=hdr_value_font)],
                  app_layout, 
                  [sg.Button('Download', font=hdr_label_font, button_color=('SpringGreen4')), sg.Button('Cancel', font=hdr_label_font)]
                 ]

        window = sg.Window('Download App', layout, modal=False)
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
                for app in self.git_app_repo.app_dict.keys():
                    if self.values["-%s-"%app] == True:
                        if self.git_app_repo.clone(app):
                            sg.popup(f'Successfully cloned {app} into {self.usr_app_path}', title='AppStore')
                        else:
                            sg.popup(f'Error cloning {app} into {self.usr_app_path}', title='AppStore Error')

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
    usr_app_path = compress_abs_path(os.path.join(os.getcwd(),'..', config.get('PATHS', 'USR_APP_PATH'))) 

    #app_store = AppStore(git_url, usr_app_path)
    #app_store.execute()
    
    
    manage_usr_apps = ManageUsrApps(usr_app_path)
    
    berry_imu = manage_usr_apps.get_app_spec('berry_imu')
    print(berry_imu.get_targets_cmake_files())
    print(berry_imu.get_startup_scr_entry())
    
    gpio_demo = manage_usr_apps.get_app_spec('gpio_demo')
    print(gpio_demo.get_targets_cmake_files())
    print(gpio_demo.get_startup_scr_entry())
    
    

