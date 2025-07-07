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
        Provide classes for managing user apps
        
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
    from eds      import AppEds
    from utils    import AppStoreDef, compress_abs_path
else:
    from .eds      import AppEds
    from .utils    import AppStoreDef, compress_abs_path

from tools import PySimpleGUI_License
import PySimpleGUI as sg


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
    CFE_TYPE_APP   = 'CFE_APP'
    CFE_TYPE_LIB   = 'CFE_LIB'
    CFE_TYPE_PROXY = 'CFE_PROXY'
   
    APP_FRAMEWORK_CFS = 'cfs'
    APP_FRAMEWORK_OSK = 'osk'  # Named based on Basecamp predecsessor called OpenSatKit 
    
    def __init__(self, app_path, app_name, proxy=False):
        self.app_path   = app_path
        self.app_name   = app_name
        self.eds_path   = os.path.join(app_path, 'eds')
        self.json_file  = os.path.join(app_path, app_name+'.json')
        self.eds_specs  = []
        self.cmd_topics = []
        self.tlm_topics = []
        self.is_valid   = False
        self.has_topics = False
        self.proxy      = proxy
        self.json       = None
        self.json_app   = None
        self.json_cfs   = None
        self.json_proxy = None
        
        # 'is_valid' and 'has_topics' are False and will be set to True as needed
        # EDS files are required for apps and optional for libraries to be valid.
        # A proxy app's should not be processed in the context of the actual app 
        # that the proxy app serves.
 
        if self.read_json_file() and not proxy:
            self.read_eds_files()

    def read_eds_files(self):
        """
        JSON spec must be loaded prior to calling this function 
        """
        # Libraries don't require an EDS spec
        if self.json_cfs['cfe-type'] == self.CFE_TYPE_LIB or self.json_cfs['cfe-type'] == self.CFE_TYPE_PROXY :
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
                    if self.json_cfs['cfe-type'] == self.CFE_TYPE_APP:
                        self.is_valid = True
        else:
            if self.json_cfs['cfe-type'] == self.CFE_TYPE_APP:
                sg.popup(f'App is missing an EDS spec. Expected {self.eds_path} to exist', title='AppStore Error', grab_anywhere=True, modal=False)
        
            
    def read_json_file(self):

        success = False
        if os.path.exists(self.json_file):
            try:
                f = open(self.json_file)
                self.json = json.load(f)
                f.close()
                success = True
            except:
                sg.popup(f'Error loading JSON spec file {self.json_file}', title='AppStore Error', grab_anywhere=True, modal=False)
        else:
            sg.popup(f'Error loading JSON spec file {self.json_file}', title='AppStore Error', grab_anywhere=True, modal=False)

        if success:
            success = False
            try:
                self.json_app = self.json['app']
                if self.proxy:
                    try:
                        self.json_proxy = self.json_app['proxy']
                        print(f'self.json_proxy: {self.json_proxy}')
                    except:
                        sg.popup(f"The JSON proxy spec file {self.json_file} does not contain the required 'proxy' object", title='AppStore Error', grab_anywhere=True, modal=False)                        
                else:
                    try:
                        self.json_cfs = self.json_app['cfs']
                    except:
                        sg.popup(f"The JSON spec file {self.json_file} does not contain the required 'cfs' object", title='AppStore Error', grab_anywhere=True, modal=False)
                success = True
            except:
                sg.popup(f"The JSON spec file {self.json_file} does not contain the required 'app' object", title='AppStore Error', grab_anywhere=True, modal=False)
        
        return success
        
    def has_topic_ids(self):
        return self.has_topics
        
    def read_eds_file(self, eds_filename):        
        eds_obj = None
        try:
            eds_obj = AppEds(eds_filename)
        except Exception as e: 
            if (self.json_cfs['cfe-type'] == self.CFE_TYPE_APP):
                sg.popup(f'Exception {repr(e)} raised when attempting to read app EDS file {eds_filename}', title='AppStore Error', grab_anywhere=True, modal=False)
            elif (self.json_cfs['cfe-type'] == self.CFE_TYPE_LIB):
                sg.popup(f'Exception {repr(e)} raised when attempting to read library EDS file {eds_filename}', title='AppStore Error', grab_anywhere=True, modal=False)
            else:
                pass
        return eds_obj        
       
    def get_app_info(self):
        info = {}
        info['title']       = self.json_app['title']
        info['version']     = self.json_app['version']
        info['supplier']    = self.json_app['supplier']
        info['url']         = self.json_app['url']
        info['description'] = self.json_app['description']
        info['requires']    = self.json_app['requires']
        info['framework']   = self.json_cfs['app-framework']
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
        files['obj-file'] = self.json_cfs['obj-file']
        files['tables']   = self.json_cfs['tables']
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
            entry_str = self.json_cfs['cfe-type']      + ', ' + \
                        self.json_cfs['obj-file']      + ', ' + \
                        self.json_cfs['entry-symbol']  + ', ' + \
                        self.json_cfs['name']          + ', ' + \
                        str(self.json_cfs['priority']) + ', ' + \
                        str(self.json_cfs['stack'])    + ', 0x0, ' + \
                        str(self.json_cfs['exception-action']) + ';' 
        except:
            sg.popup(f'Error creating targets.cmake entry due to missing or malformed JSON file.\nPartial entry string = {self.json_file}', title='AppStore Error', grab_anywhere=True, modal=False)
        
        return entry_str


    def get_real_github_repo(self):
        """
        Return the URL and and branch-tag of the "real" repo be cloned
        
        The relevant proxy app spec "proxy" object are:
            
            "proxy": {
                "url": "URL of real github repo",
                "branch-tag": "specific tag to be cloned",
                ...
            },
        """
        proxy_github_repo = ()
        valid_github_repo = False
        print(f'self.json_file = {self.json_file}')
        url = self.json_proxy['url']
        try:
            print(f'@@self.json_proxy = {self.json_proxy}')
            url = self.json_proxy['url']
            print(f'url: {url}')
            try:
                branch_tag = self.json_proxy['branch-tag']
                print(f'branch_tag: {branch_tag}')        
                valid_github_repo = True
            except:
                sg.popup(f"The JSON spec file {self.json_file} does not contain the required 'proxy->branch-tag' object", title='AppStore Error', grab_anywhere=True, modal=False)
        except:
            sg.popup(f"The proxy JSON spec file {self.json_file} does not contain the required 'proxy->url' object", title='AppStore Error', grab_anywhere=True, modal=False)
        
        if valid_github_repo:
            proxy_github_repo = (url,branch_tag) 
        
        return proxy_github_repo


    def get_proxy_file_list(self):
        """
        Return an array of strings and each string identifies a file that needs
        to be copied from the proxy repo to the actual app's repo. 
        
        The relevant proxy app spec "proxy" object are:
             "proxy": {
                ...
                # Array of files to be copied (overwrite if needed) from proxy into actual app's repo             
                "update-files": [
                    "sample_app.json    >> sample_app.json",
                    "CMakeLists.txt     >> CMakeLists.txt",
                    "eds/sample_app.xml >> eds/sample_app.xml" 
                ]
            },
        """
        proxy_file_list = []
        valid_github_repo = False
        print(f'self.json_file = {self.json_file}')
        try:
            proxy_file_list = self.json_proxy['update-files']
            valid_github_repo = True
        except:
            sg.popup(f"The proxy JSON spec file {self.json_file} does not contain the required 'proxy->update-files' object", title='AppStore Error', grab_anywhere=True, modal=False)
                
        return proxy_file_list

            
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
            if not app_name.startswith(AppStoreDef.PROXY_APP_PREFIX):
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

if __name__ == '__main__':

    config = configparser.ConfigParser()
    config.read('../basecamp.ini')

    git_url = config.get('APP','APP_STORE_URL')
    usr_app_abs_path = compress_abs_path(os.path.join(os.getcwd(),'..', config.get('PATHS', 'USR_APP_PATH'))) 
    
    manage_usr_apps = ManageUsrApps(usr_app_abs_path)
    
    berry_imu = manage_usr_apps.get_app_spec('berry_imu')
    print(berry_imu.get_targets_cmake_files())
    print(berry_imu.get_startup_scr_entry())
    
    gpio_demo = manage_usr_apps.get_app_spec('gpio_demo')
    print(gpio_demo.get_targets_cmake_files())
    print(gpio_demo.get_startup_scr_entry())
    
    

