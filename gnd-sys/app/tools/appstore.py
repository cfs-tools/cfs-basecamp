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
        Provide classes that manage downloading and installing apps from github repos
        
    Notes:    
        Assumes the exact same app name is used for
        - App directory
        - App Electronic Data Sheet (EDS) file
        - App cFS spec JSON file 
        - Proxy app name following AppStoreDef.PROXY_APP_PREFIX
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
    from eds     import AppEds
    from usrapps import AppSpec
    from utils   import AppStoreDef, compress_abs_path
else:
    from .eds     import AppEds
    from .usrapps import AppSpec
    from .utils   import AppStoreDef, compress_abs_path

from tools import PySimpleGUI_License
import PySimpleGUI as sg


###############################################################################

class GitHubAppRepo():
    """   
    """
    def __init__(self, git_url, release_tag, usr_app_rel_path, quiet_ops=False):
        """
        usr_app_rel_path  - Relative path to where git repos should be cloned into
        
        quiet_ops: Used for non-GUI scenarios when autonomous/silent operations
        is needed. Error dialogues are still displayed. 
        """
        self.usr_clone_path = usr_app_rel_path
        self.git_url     = git_url
        self.release_tag = release_tag
        self.quiet_ops   = quiet_ops
        self.app_repo    = None
        self.app_dict    = {}
         
         
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
        
    def get_target_dir(self, app_name):
        """
        Return directory that was cloned into.
        """
        return compress_abs_path(os.path.join(os.getcwd(), self.usr_clone_path, app_name))
        
    
    def clone_repo(self, git_url, app_name, display_success_popup=True):
        """
        """
        ret_status = False
        clone_repo = True
        target_dir = self.get_target_dir(app_name)
        if os.path.exists(target_dir):
            overwrite = sg.popup_yes_no(f"{target_dir} exists. Do you want to overwrite it?",  title="AppStore")
            if (overwrite == 'Yes'):
                shutil.rmtree(target_dir)
            else:
                clone_repo = False
                ret_status = True
            
        if clone_repo:
            saved_cwd = os.getcwd()
            os.chdir(self.usr_clone_path)
            if display_success_popup:
                sg.popup_quick_message('Github repo cloning in progress. Please be patient...', auto_close_duration=5)
            if len(self.release_tag) == 0:
                sys_status = os.system(f'git clone {git_url}')
            else:
                sys_status = os.system(f'git clone --branch {self.release_tag} {git_url}')
            if (sys_status == 0):
                if display_success_popup:
                    sg.popup(f'Successfully cloned {app_name} into {target_dir}', title='AppStore')
                ret_status = True
            else:
                sg.popup(f'Error cloning {app_name} into {target_dir}', title='AppStore Error')
                ret_status = False
            os.chdir(saved_cwd)

        return ret_status


    def clone_basecamp_repo(self, app_name, display_success_popup=True):
        """
        """
        ret_status = False
        if app_name in self.app_dict:
            url = self.app_dict[app_name]['clone_url']
            ret_status = self.clone_repo(url, app_name, display_success_popup)
        
        return ret_status

 
    def clone_proxy_repo(self, proxy_app_name):
        """
        A Basecamp proxy app contains an app spec with information for cloning a non-Basecamp repo
        and updating the other party's repo with files that allow it to be integrated into Basecamp.
        The external repos are typically NASA cFS app repos.
        
        This a helper function only used by clone() so it can assume the cwd is in user apps folder
                
        When this function is called the proxy app repo has been cloned. The remaining steps
        are:
          1. Read proxy app spec
          2. Clone actual app 
          3. Use proxy app spec to copy files into actual app's repo
        
        """
        ret_status = False
        proxy_app_dir = self.get_target_dir(proxy_app_name)
        real_app_name = proxy_app_name[len(AppStoreDef.PROXY_APP_PREFIX):]
        real_app_dir  = self.get_target_dir(real_app_name)
        print(f'\n****proxy_app_dir: {proxy_app_dir}, proxy_app_name: {proxy_app_name}')
        print(f'****real_app_dir: {real_app_dir}, real_app_name: {real_app_name}')

        cloned_proxy = self.clone_basecamp_repo(proxy_app_name, display_success_popup=False)
        print(f'cloned_proxy: {cloned_proxy}')
        if cloned_proxy:
            proxy_app_spec   = AppSpec(proxy_app_dir, proxy_app_name, proxy=True)
            real_github_repo = proxy_app_spec.get_real_github_repo()
            print(f'****real_github_repo: {real_github_repo}')
            if len(real_github_repo) == 2:
                git_app_repo = GitHubAppRepo(real_github_repo[0],real_github_repo[1], self.usr_clone_path)
                ret_status = git_app_repo.clone_repo(real_github_repo[0], real_app_name, display_success_popup=(not self.quiet_ops))
                print(f'\nReal clone ret_status: {ret_status}')
                if ret_status:
                    proxy_file_list = proxy_app_spec.get_proxy_file_list()
                    print(f'\n>>>proxy_file_list = {proxy_file_list}')
                    # No files is treated as invalid. This could change if Basecamp becomes wildly
                    # adopted and compliant third party repos flood the internet :-)
                    if len(proxy_file_list) > 0:                    
                        ret_status = self.copy_proxy_files(proxy_app_dir, real_app_dir, proxy_file_list)
                    else:
                        ret_status = False

        if ret_status:
            sg.popup(f'Successfully cloned and populated {real_app_name} from {proxy_app_name}', title='AppStore')
        else:
            sg.popup(f'Error cloning and populating {real_app_name} from {proxy_app_name}', title='AppStore Error')

        return ret_status


    def copy_proxy_files(self, proxy_app_dir, real_app_dir, proxy_file_list):
        """
        """
        ret_status = False
        for proxy_file in proxy_file_list:
            proxy_file = proxy_file.split(AppStoreDef.PROXY_FILE_COPY_TOKEN)
            src_pathfile = os.path.join(proxy_app_dir, proxy_file[0].strip())
            dst_pathfile = os.path.join(real_app_dir,  proxy_file[1].strip())            
            os.makedirs(os.path.dirname(dst_pathfile), exist_ok=True)
            print(f'src_pathfile: {src_pathfile}')
            print(f'dst_pathfile: {dst_pathfile}')
            try:
                print(f'\n>>>Copying {src_pathfile} {AppStoreDef.PROXY_FILE_COPY_TOKEN} {dst_pathfile}')
                shutil.copyfile(src_pathfile, dst_pathfile)        
                ret_status = True
            except Exception as e:     
                ret_status = False                
                sg.popup(f'Error copying proxy file from\n   {src_pathfile}\nto  {dst_pathfile}\n{e}', title='AppStore Error')
        
        return ret_status


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

    def clone_old(self, app_name, proxy=False, success_popup=True):
        """
        """
        ret_status = False
        if app_name in self.app_dict or proxy:
            clone_repo = True
            target_dir = compress_abs_path(os.path.join(os.getcwd(), self.usr_clone_path, app_name))
            if os.path.exists(target_dir):
                overwrite = sg.popup_yes_no(f"{target_dir} exists. Do you want to overwrite it?",  title="AppStore")
                if (overwrite == 'Yes'):
                    shutil.rmtree(target_dir)
                else:
                    clone_repo = False
                    ret_status = True
            
            if clone_repo:
                saved_cwd = os.getcwd()
                os.chdir(self.usr_clone_path)
                clone_url = self.app_dict[app_name]["clone_url"]
                sys_status = os.system("git clone {}".format(self.app_dict[app_name]["clone_url"]))
                if (sys_status == 0):
                    if app_name.startswith(AppStoreDef.PROXY_APP_PREFIX):
                        ret_status = self.clone_proxy_app(target_dir, app_name)
                    else:
                        if success_popup:
                            sg.popup(f'Successfully cloned {app_name} into {target_dir}', title='AppStore')
                else:
                    sg.popup(f'Error cloning {app_name} into {target_dir}', title='AppStore Error')
                    ret_status = False
                os.chdir(saved_cwd)
        return ret_status

              
###############################################################################

class AppStore():
    """
    Manage the user interface for downloading apps from github and cloning
    them into the user's app directory. 
    """
        
    def __init__(self, git_url, usr_app_rel_path, git_topic_include, git_topic_exclude, app_group):
        """
        git_topic_include - List of github topics identifying repos to be included
        git_topic_exclude - List of github topics identifying repos to be excluded
        """
        self.git_topic_include = git_topic_include
        self.git_topic_exclude = git_topic_exclude 
        self.app_group = app_group
        self.usr_app_abs_path = compress_abs_path(os.path.join(os.getcwd(), usr_app_rel_path))
        self.git_app_repo = GitHubAppRepo(git_url, AppStoreDef.BASECAMP_REPO_BRANCH, usr_app_rel_path)
        self.git_app_repo_keys = [] # keys of app repos that pass the include/exclude filters 
        self.window  = None

        
    def create_window(self):
        """
        """
        hdr_label_font = ('Arial bold',12)
        hdr_value_font = ('Arial',11)
        window_width   = 110
        app_layout = []
        for app in self.git_app_repo.app_dict.keys():
            topics = self.git_app_repo.get_topics(app)
            if any(x in topics for x in self.git_topic_include) and not any(x in topics for x in self.git_topic_exclude):
                self.git_app_repo_keys.append(app)
                app_layout.append([sg.Checkbox(app.upper(), default=False, font=hdr_label_font, size=(18,0), key=f'-{app}-'),  
                                  sg.Text(self.git_app_repo.get_descr(app), font=hdr_value_font, size=(window_width,1))])
                
        layout = [
                  [sg.Text("Select one or more apps to download and click the <Download> button to add them to the usr/app repositories:", font=hdr_label_font, size=(window_width,None))],
                  [sg.Text("   - Follow the steps in 'File->Add App to Target' to add the apps/libs to add the cFS target", font=hdr_value_font)],
                  [sg.Text("   - The Hello World tutorial in 'Tutorials->Create App Tool' describes the steps to add an app to a cFS target", font=hdr_value_font)],
                  [sg.Text("   - An app's JSON spec file has a 'requires' parameter that identifies dependencies that must be installed prior to the app\n", font=hdr_value_font)],
                  [app_layout],
                  [sg.Text("")],
                  [sg.Button('Download', font=hdr_label_font, button_color=('SpringGreen4'), pad=(2,0)), sg.Button('Cancel', font=hdr_label_font, pad=(2,0))]
                 ]

        window = sg.Window(f'Download {self.app_group} App', layout, modal=False)
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
                        # Clone functions report status to user via popups
                        if app.startswith(AppStoreDef.PROXY_APP_PREFIX):
                            ret_status = self.git_app_repo.clone_proxy_repo(app)
                        else:
                            ret_status = self.git_app_repo.clone_basecamp_repo(app) 
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

    GIT_URL          = config.get('APP','APP_STORE_URL')
    USR_APP_REL_PATH = config.get('PATHS', 'USR_APP_PATH')
    usr_app_abs_path = compress_abs_path(os.path.join(os.getcwd(),'..', USR_APP_REL_PATH)) 

    app_store = AppStore(GIT_URL, usr_app_abs_path)
    app_store.execute()
