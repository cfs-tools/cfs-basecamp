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
import os
import errno
import configparser
import shutil
import requests

import logging
logger = logging.getLogger(__name__)

if __name__ == '__main__':
    from utils    import compress_abs_path
else:
    from .utils   import compress_abs_path

from tools import PySimpleGUI_License
import PySimpleGUI as sg


###############################################################################

class GitHubApps():
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
        

    def clone(self, app_name, quiet_ops=False):
        """
        quiet_ops: Used for non-GUI scenarios and autonomous/silent operations
        is needed. Error dialogues are still displayed. 
        """
        if app_name in self.app_dict:
            clone_repo = True
            target_dir = compress_abs_path(os.path.join(os.getcwd(), self.usr_clone_path, app_name))
            if os.path.exists(target_dir):
                if quiet_ops:
                   shutil.rmtree(target_dir)
                else:
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
                    if not quiet_ops:
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

if __name__ == '__main__':

    config = configparser.ConfigParser()
    config.read('../basecamp.ini')

    GIT_URL          = config.get('APP','APP_STORE_URL')
    USR_APP_REL_PATH = config.get('PATHS', 'USR_APP_PATH')   
    usr_app_dir      = os.path.join(os.getcwd(),'..', USR_APP_REL_PATH) 
    
    git_apps = GitHubApps(GIT_URL, usr_app_dir)
    git_apps.create_dict()
    apps = ['pi_iolib']
    for app in apps:
       print(f'Cloning app {app}')
       git_apps.clone(app)    


