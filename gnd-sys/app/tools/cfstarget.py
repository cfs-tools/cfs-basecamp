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
        Define classes for managing a cFS target
        
    Notes:
      1. Assumes the exact same app name is used for
         - App directory
         - App Electronic Data Sheet (EDS) file
         - App cFS spec JSON file 
         - Proxy app name following AppStoreSpec.PROXY_APP_PREFIX
      2. Proxy apps are not removed when the 'real' app is removed.
"""

import sys
import time
import os
import json
import shutil
import subprocess
import threading
from enum import Enum

import logging
logger = logging.getLogger(__name__)

if __name__ == '__main__':
    from eds      import AppEds, CfeTopicIds
    from jsonfile import JsonTblTopicMap
    from usrapps  import AppSpec, ManageUsrApps
    from utils    import compress_abs_path
else:
    from .eds      import AppEds, CfeTopicIds
    from .jsonfile import JsonTblTopicMap
    from .usrapps  import AppSpec, ManageUsrApps
    from .utils    import compress_abs_path
    
from tools import PySimpleGUI_License
import PySimpleGUI as sg

###############################################################################

DEFAULT_TARGET_NAME = 'cpu1'                # TODO: Parameterize
CFS_DEFS_FOLDER     = 'basecamp_defs'
INSERT_KEYWORD      = '!BASECAMP-INSERT!'
CFE_STARTUP_SCR     = 'cfe_es_startup.scr'

# Basecamp's default configuration supports these apps so they can't be removed
# like other app store apps
RESERVED_APPS       = ['sample_app']

###############################################################################

def build_cfs_target(cfs_build_script, cfs_abs_base_path, main_window):

    build_subprocess = subprocess.Popen(f'{cfs_build_script} {cfs_abs_base_path}',
                       stdout=subprocess.PIPE, shell=True, bufsize=1, universal_newlines=True)
    if build_subprocess is not None:
        cfs_stdout = CfsStdout(build_subprocess, main_window)
        cfs_stdout.start()

    return build_subprocess


###############################################################################

class Cfs():

    # Shell script names should not change and are considered part of the application
    # Therefore they can be defined here and not in a configuration file

    SH_BUILD_CFS_TOPICIDS = './build_cfs_topicids.sh'
    SH_MAKE_INSTALL_CFS   = './make_install.sh'
    SH_STOP_CFS           = './stop_cfs.sh'
    SH_START_CFS          = './start_cfs.sh'
    SH_SUDO_START_CFS     = './sudo_start_cfs.sh'


###############################################################################

class AppTargetStatus():
    """
    Helper class for ManageCfs
    """
    def __init__(self):
        self.app_in_applist        = {'state': True, 'descr': 'Valid'}  # App defined in targets.cmake APPLIST
        self.tbl_in_filelist       = {'state': True, 'descr': 'Valid'}  # Tables defined in targets.cmake FILELIST
        self.tbl_files_in_defs     = {'state': True, 'descr': 'Valid'}  # Tables in basecamp_defs folder
        self.app_in_startup_scr    = {'state': True, 'descr': 'Valid'}  # App defined in CFE_STARTUP_SCR
        self.depend_in_applist     = {'state': True, 'descr': 'Valid'}  # Dependencies in targets.cmake APPLIST
        self.depend_in_startup_scr = {'state': True, 'descr': 'Valid'}  # Dependencies in CFE_STARTUP_SCR prior to app
    def print(self):
        print('\nAppTargetStatus:')
        print(f"app_in_applist:        {self.app_in_applist['state']}, {self.app_in_applist['descr']}")
        print(f"tbl_in_filelist:       {self.tbl_in_filelist['state']}, {self.tbl_in_filelist['descr']}")
        print(f"tbl_files_in_defs:     {self.tbl_files_in_defs['state']}, {self.tbl_files_in_defs['descr']}")
        print(f"app_in_startup_scr:    {self.app_in_startup_scr['state']}, {self.app_in_startup_scr['descr']}")
        print(f"depend_in_applist:     {self.depend_in_applist['state']}, {self.depend_in_applist['descr']}")
        print(f"depend_in_startup_scr: {self.depend_in_startup_scr['state']}, {self.depend_in_startup_scr['descr']}")
    def all_true(self):
        return all([self.app_in_applist['state'],self.tbl_in_filelist['state'],self.tbl_files_in_defs['state'],
                    self.app_in_startup_scr['state'],self.depend_in_applist['state'],self.depend_in_startup_scr['state']])
    def all_false(self):
        return all([not self.app_in_applist['state'],not self.tbl_in_filelist['state'],not self.tbl_files_in_defs['state'],
                    not self.app_in_startup_scr['state'],not self.depend_in_applist['state'],not self.depend_in_startup_scr['state']])
    def app_not_installed(self):
        """
        Check fields that indicate an app is installed. It's not perfect but reasonable. 
        """
        return all([not self.app_in_applist['state'],not self.tbl_in_filelist['state'],not self.tbl_files_in_defs['state'], not self.app_in_startup_scr['state']])
    def invalidate_all(self):
        self.app_in_applist['state'] = False
        self.app_in_applist['descr'] = 'Invalid'
        self.tbl_in_filelist['state'] = False
        self.tbl_in_filelist['descr'] = 'Invalid'
        self.tbl_files_in_defs['state'] = False
        self.tbl_files_in_defs['descr'] = 'Invalid'
        self.app_in_startup_scr['state'] = False
        self.app_in_startup_scr['descr'] = 'Invalid'
        self.depend_in_applist['state'] = False
        self.depend_in_applist['descr'] = 'Invalid'
        self.depend_in_startup_scr['state'] = False
        self.depend_in_startup_scr['descr'] = 'Invalid'

    
###############################################################################

class AppTopicIdStatus():
    """
    Helper class for ManageCfs
    """
    def __init__(self):
        self.ini_topics_defined = {'state': True, 'descr': 'Valid'}  # Topic IDs used in ini table are defined in cfe-topicids.xml
        self.topics_in_kit_to   = {'state': True, 'descr': 'Valid'}  # App's topic IDs are defined in KIT_TO's filter table
    def print(self):
        print('AppTopicStatus:')
        print(f"ini_topics_defined: {self.ini_topics_defined['state']}, {self.ini_topics_defined['descr']}")
        print(f"topics_in_kit_to:   {self.topics_in_kit_to['state']}, {self.topics_in_kit_to['descr']}")

###############################################################################
  
class CfsStdout(threading.Thread):
    """
    """
    def __init__(self, cfs_subprocess, window):
        threading.Thread.__init__(self)
        self.cfs_subprocess = cfs_subprocess
        self.window = window
        self.cfs_subprocess_log = ""
        self.daemon = True
        
    def run(self):
        """
        This function is invoked after a cFS process is started and it's design depends on how Popen is
        configured when the cFS process is started. I've tried lots of different designs to make this 
        non-blocking and easay to terminate. It assumes the the Popen parameters bufsize=1 and
        universal_newlines=True (text output). A binary stdout would need line.decode('utf-8'). Some loop
        design options:
            for line in io.TextIOWrapper(self.cfs_subprocess.stdout, encoding="utf-8"):
                self.cfs_subprocess_log += line
            while True:
                line = self.cfs_subprocess.stdout.readline()
                if not line:
                    break
                self.cfs_subprocess_log += line

            for line in iter(self.cfs_subprocess.stdout.readline, ''):
                print(">>Line: " + line)
                self.cfs_subprocess_log += line

        Reading stdout is a blocking function. The current design does not let the process get killed and I
        think it's because the read function is always active. I put the try block there because I'd like to
        add an exception mechanism to allow the thread to be terminated. Subprocess communicating with a timeout
        is not an option because the child process is terminated if a timeout occurs. I tried the pseudo terminal
        module as an intermediator between the cFS process and stdout thinking it may be non-blocking but
        it still blocked. 
        
        """
 
        try:
            logger.info("Starting cFS terminal window stdout display")
            for line in iter(self.cfs_subprocess.stdout.readline, ''):
                #print(">>Line: " + line)
                self.cfs_subprocess_log += line
                self.window["-CFS_PROCESS_TEXT-"].update(self.cfs_subprocess_log)
                self.window["-CFS_PROCESS_TEXT-"].set_vscroll_position(1.0)  # Scroll to bottom (most recent entry)
        except Exception as e:
            logger.error("Starting cFS terminal window stdout display exception\n" + str(e))
            
    def get_id(self):
 
        # returns id of the respective thread
        if hasattr(self, '_thread_id'):
            return self._thread_id
        for id, thread in threading._active.items():
            if thread is self:
                return id
  
    def terminate(self):
        """
        Terminate the thread by rasing an exception
        """
        logger.info("Raising CfsStdout exception to terminate thread")
        thread_id = self.get_id()
        res = ctypes.pythonapi.PyThreadState_SetAsyncExc(thread_id,
              ctypes.py_object(SystemExit))
        if res > 1:
            ctypes.pythonapi.PyThreadState_SetAsyncExc(thread_id, 0)
            print('Exception raise failure')


###############################################################################

class ManageCfs():
    """
    Manage the display for configuring, building and running the cFS.
    app_abs_path is the python application path, not cFS apps
    #TODO - Define path and file constants. ManageCfs should be its own package with cFS constants.
    #TODO - Consolidate common logic in restore_targets_cmake(), in_targets_cmake_list(() and update_targets_cmake()
    """
    TRI_STATE = Enum('TriState', ['NOT_APP', 'TRUE', 'FALSE'])

    def __init__(self, basecamp_abs_path, cfs_abs_base_path, usr_app_rel_path, main_window, cfs_target):
        self.basecamp_abs_path      = basecamp_abs_path
        self.cfs_abs_base_path      = cfs_abs_base_path
        self.cfs_abs_defs_path      = os.path.join(self.cfs_abs_base_path, 'basecamp_defs')
        self.basecamp_tools_path    = os.path.join(basecamp_abs_path, 'tools')
        self.usr_app_path           = compress_abs_path(os.path.join(basecamp_abs_path, usr_app_rel_path))
        self.main_window            = main_window
        self.cfs_target             = cfs_target
        self.startup_scr_filename   = cfs_target + '_' + CFE_STARTUP_SCR
        self.startup_scr_file       = os.path.join(self.cfs_abs_defs_path, self.startup_scr_filename)
        self.targets_cmake_filename = 'targets.cmake'
        self.targets_cmake_file     = os.path.join(self.cfs_abs_defs_path, self.targets_cmake_filename)
        self.cfe_topic_id_filename  = 'cfe-topicids.xml'
        self.cfe_topic_id_file      = os.path.join(self.cfs_abs_defs_path, 'eds', self.cfe_topic_id_filename)
        self.kit_to_tbl_filename    = cfs_target + '_' + 'kit_to_pkt_tbl.json'
        self.kit_to_tbl_file        = os.path.join(self.cfs_abs_defs_path, self.kit_to_tbl_filename)
        self.cmake_app_list         = cfs_target + '_APPLIST'
        self.cmake_file_list        = cfs_target + '_FILELIST'
        self.build_subprocess       = None
        self.selected_app           = None
        self.usr_app_spec           = None
        self.usr_app_install_detail = None
        
        self.b_size  = (4,1)
        self.b_pad   = ((0,2),(2,2))
        self.b_font  = ('Arial bold', 12)
        self.b_color = 'black on LightSkyBlue3'
        self.t_size  = (2,1)
        self.t_font  = ('Arial', 12)
        self.step_font  = ('Arial bold', 14)

    def add_usr_app_gui(self, usr_app_list):
        """
        Provide steps for the user to integrate an app. Allow the user to add
        multiple apps before moving onto the build step. 
        The steps have some degree of independence in case the user doesn't do
        things in order which means some processing may be repeated. For example
        the table files are recomputed for the edit targets.cmake step and the
        copy files to cFS '_defs' steps. 
        """
        #TODO - Use a loop to construct the layout

        layout = [
                  [sg.Text("Perform the following steps to add one or more apps to the cFS target. For step 1, choose 'Auto' to automatically\nperform all of the steps or 'Man' to manually perform each step. Libraries MUST be added prior\nto the apps that depend upon them.\n", font=self.t_font)],
                  
                  [sg.Text('1. Add app to the cFS build configuration', font=self.step_font, pad=self.b_pad)],
                  [sg.Text('', size=self.b_size), sg.Combo(usr_app_list, pad=self.b_pad, font=self.b_font, enable_events=True, key="-USR_APP-", default_value=usr_app_list[0]),
                   sg.Text('Select an app from the dropdown list', font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Button('Auto', size=self.b_size, button_color=('SpringGreen4'), font=self.b_font, pad=self.b_pad, enable_events=True, key='-1_AUTO-'),
                   sg.Text('Automatically perform all steps or ...', font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Button('Man',  size=self.b_size, button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-1A_MAN-'),
                   sg.Text('Copy table files to %s' % CFS_DEFS_FOLDER, font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Button('Man',  size=self.b_size, button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-1B_MAN-'),
                   sg.Text("Update targets.cmake's %s and %s" % (self.cmake_app_list, self.cmake_file_list), font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Button('Man',  size=self.b_size, button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-1C_MAN-'),
                   sg.Text(f'Update cpu1_{CFE_STARTUP_SCR}', font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Button('Man',  size=self.b_size, button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-1D_MAN-'),
                   sg.Text('Update EDS cfe-topicids.xml', font=self.t_font)], 
                  [sg.Text('', size=self.b_size), sg.Button('Man',  size=self.b_size, button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-1E_MAN-'),
                   sg.Text('Update telemetry output app table', font=self.t_font)],
                  
                  [sg.Text('2. Build new cFS target', font=self.step_font, pad=self.b_pad)],
                  [sg.Text('', size=self.b_size), sg.Button('Build New', size=(8,1), button_color=('SpringGreen4'), font=self.b_font, pad=self.b_pad, enable_events=True, key='-2_AUTO-')],
                  
                  [sg.Text('3. Stop the cFS if it is running', font=self.step_font, pad=self.b_pad)],
                  [sg.Text('', size=self.b_size), sg.Text('Close this window and click <Stop cFS> from the main window or ...', font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Text('Open a terminal window & kill the cFS process or ...', font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Text('Submit [sudo] password and click <Submit>', font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Button('Submit', size=(6,1), button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-3_AUTO-'),
                   sg.InputText(password_char='*', size=(15,1), font=self.t_font, pad=self.b_pad, key='-PASSWORD-')],

                  [sg.Text('4. Exit and restart Basecamp', font=self.step_font, pad=self.b_pad)],
                  [sg.Text('', size=self.b_size), sg.Button('Restart', size=(6,1), button_color=('SpringGreen4'), font=self.b_font, pad=self.b_pad, enable_events=True, key='-4_AUTO-')],
                 ]
        # sg.Button('Exit', enable_events=True, key='-EXIT-')
        window = sg.Window('Add App to Target', layout, resizable=True, finalize=True) # modal=True)

        restart_main_window = False
        while True:
        
            self.event, self.values = window.read(timeout=200)
        
            if self.event in (sg.WIN_CLOSED, 'Exit', '-EXIT-') or self.event is None:
                break

            ## Step 1 - Update cFS build configuration with selected app
            
            elif self.event == '-1_AUTO-': # Autonomously perform step 1
                """
                Errors are reported in a popup by each function. The success string is an aggregate of each successful return
                that will be reported in a single popup. 
                A boolean return value of True from each function indicates there weren't any errors, it doesn't mean a paricular
                update was performed, because the update may not be required.
                """ 
                self.selected_app = self.values['-USR_APP-']
                req_libs_installed, missing_req_libs = self.verify_dependencies_installed(self.selected_app)
                if req_libs_installed:
                    self.add_usr_app(self.selected_app)
                else:
                    status_text = f'{self.selected_app.upper()} requires the following libraries to be installed:\n  {missing_req_libs}\n'
                    sg.popup(status_text, title=f'Missing {self.selected_app.upper()} Libraries', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)

            elif self.event == '-1A_MAN-':
                self.selected_app = self.values['-USR_APP-']
                self.usr_app_spec = self.manage_usr_apps.get_app_spec(self.selected_app)
                app_info = self.usr_app_spec.get_app_info()
                cfe_obj_type  = app_info['obj-type']
                app_framework = app_info['framework']
                self.copy_app_tables(cfe_obj_type, app_framework, auto_copy=False)  # Copy table files from app dir to cFS '_defs' file
            elif self.event == '-1B_MAN-':
                self.update_targets_cmake(auto_update=False)
            elif self.event == '-1C_MAN-':
                self.update_startup_scr(auto_update=False)
            elif self.event == '-1D_MAN-':
                popup_text = f"After this dialogue, {self.cfe_topic_id_filename} will open in an editor.\n Replace spare topic IDs with the app's topic ID names"
                sg.popup(popup_text, title=f'Update {self.cfe_topic_id_filename}', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
                self.text_editor = sg.execute_py_file('texteditor.py', parms=self.cfe_topic_id_file, cwd=self.basecamp_tools_path)
            elif self.event == '-1E_MAN-':
                popup_text = f"After this dialogue, {self.kit_to_tbl_filename} will open in an editor.\n Replace spare topic IDs with the app's topic ID names"
                sg.popup(popup_text, title=f'Update {self.kit_to_tbl_filename}', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
                self.text_editor = sg.execute_py_file('texteditor.py', parms=self.kit_to_tbl_file, cwd=self.basecamp_tools_path)
                
            ## Step 2 - Build the cFS

            elif self.event == '-2_AUTO-': # Build the cFS
                self.build_target()
            
            elif self.event == '-2_MAN-': # Build the cFS
                popup_text = f"Open a terminal window, change directory to {self.cfs_abs_base_path} and build the cFS. See '{Cfs.SH_BUILD_CFS_TOPICIDS}' for guidance"
                sg.popup(popup_text, title='Manually Stop the cFS', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)   

            ## Step 3 - Stop the cFS prior to modifying or adding an app
            
            elif self.event == '-3_AUTO-': # Stop the cFS prior to modifying or adding an app
                """
                #todo: 
                #1 - Current window blocked this from working.
                self.main_window['-STOP_CFS-'].click()
                #2 - Couldn't get blocking and tlm threading exceptions worked out
                layout = [[sg.Text("Enter [sudo] password", size=(20,1)), sg.InputText(password_char='*')],
                          [sg.Button("Submit"), sg.Button("Cancel")]]
                window = sg.Window("Stop cFS", layout) #, modal=True)
                event,values = window.read()
                window.close()
                password = values[0]
                status = subprocess.run(Cfs.SH_STOP_CFS, shell=True, cwd=self.basecamp_abs_path, input=password.encode())
                """
                #popup_text = 'After you close this popup. Enter your sudo password in the terminal where you started cfs-basecamp'
                #sg.popup(popup_text, title='Automatically Stop the cFS', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=True)
                password = self.values['-PASSWORD-']
                if password is not None:
                    status = subprocess.run(Cfs.SH_STOP_CFS, shell=True, cwd=self.basecamp_abs_path, input=password.encode())
                    print('status type = %s'%str(type(status.returncode)))
                    if status.returncode == 0:
                        popup_text = f"'{Cfs.SH_STOP_CFS}' successfully executed with return status {status.returncode}"
                        self.main_window['-STOP_CFS-'].click()
                    else:
                        popup_text = f"'{Cfs.SH_STOP_CFS}' returned with error status {status.returncode}"
                    sg.popup(popup_text, title='Stop the cFS', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
                else:
                    popup_text = 'No attempt to stop the cFS, since no password supplied'
                    sg.popup(popup_text, title='Stop the cFS', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
                            
            elif self.event == '-3_MAN-': # Stop the cFS prior to modifying or adding an app
                popup_text = f"Open a terminal window and kill any running cFS processes. See '{Cfs.SH_STOP_CFS}' for guidance" 
                sg.popup(popup_text, title='Stop the cFS', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)

            ## Step 4 - Restart Basecamp

            elif self.event == '-4_AUTO-': # Reload cFS python EDS definitions
                if self.restart_main_gui(''):
                    restart_main_window = True
                break
                
        window.close()       
        if restart_main_window:
            self.main_window['-RESTART-'].click()

    def add_usr_app(self, usr_app, quiet_ops=False):
        self.selected_app = usr_app
        self.usr_app_spec = self.manage_usr_apps.get_app_spec(self.selected_app)
        status_text = f"{self.selected_app.upper()} was successfully added to Basecamp's cFS target:\n\n"
        app_info = self.usr_app_spec.get_app_info()
        cfe_obj_type  = app_info['obj-type']
        app_framework = app_info['framework']
        display_auto_popup = False 
        copy_tables_passed, copy_tables_text = self.copy_app_tables(cfe_obj_type, app_framework, auto_copy=True)
        if copy_tables_passed:
            status_text += f'1. {copy_tables_text}\n\n' 
            update_cmake_passed, update_cmake_text = self.update_targets_cmake(auto_update=True)
            if update_cmake_passed:
                status_text += f'2. {update_cmake_text}\n\n'
                update_startup_passed, update_startup_text = self.update_startup_scr(auto_update=True)
                if update_startup_passed:
                    status_text += f'3. {update_startup_text}\n\n'
                    if self.usr_app_spec.has_topic_ids():
                        update_topics_passed, update_topics_text = self.update_topic_ids()
                        status_text += f'4. {update_topics_text}\n\n'
                        display_auto_popup = update_topics_passed
                    else:
                        status_text += f"4. Topic IDs not updated since it's a library\n\n"
                        display_auto_popup = True
        if display_auto_popup and not quiet_ops:
            sg.popup(status_text, title=f'Update {self.startup_scr_filename}', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
        return status_text
    
    def remove_usr_app_gui(self, app_name_list):

        no_yes = ['No', 'Yes']
        layout = [
                  [sg.Text(f'Select app to remove: ', font=self.t_font),
                   sg.Combo(app_name_list, pad=self.b_pad, font=self.b_font, enable_events=True, key="-USR_APP-", default_value=app_name_list[0])],
                  [sg.Text('Do you want to remove the source files from usr/apps? ', font=self.t_font),
                   sg.Combo(no_yes, font=self.b_font, enable_events=True, key='-DELETE_FILES-', default_value=no_yes[0], pad=((0,5),(5,5)))], 
                  [sg.Text('', font=self.t_font)],
                  [sg.Button('Remove App', button_color=('SpringGreen4'), pad=(2,0)), sg.Cancel(button_color=('gray'), pad=(2,0))]
                 ]
        
        window = sg.Window('Remove App from Target', layout, resizable=True, finalize=True)
        while True: # Event Loop
            self.event, self.values = window.read()
            if self.event in (sg.WIN_CLOSED, 'Cancel') or self.event is None:       
                break
            if self.event == 'Remove App':
                self.selected_app = self.values['-USR_APP-']
                self.usr_app_spec = self.manage_usr_apps.get_app_spec(self.selected_app)
                self.remove_app_tables()
                self.restore_targets_cmake()
                self.restore_startup_scr()
                if self.usr_app_spec.has_topic_ids():
                    self.restore_topic_ids()                
                if self.values['-DELETE_FILES-'] == 'Yes':
                    self.remove_app_src_files()
                sg.popup(f'Successfully removed {self.selected_app.upper()}', title='Remove App', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
                break

        window.close()
        
    def usr_app_status_gui(self, app_name_list):

        hdr_label_font = ('Arial bold',12)
        hdr_value_font = ('Arial',12)
        app_layout = []
        app_key_list = []
        app_status = {}
        for app in app_name_list:
            app_info = self.manage_usr_apps.get_app_spec(app).get_app_info()           
            install_status = self.app_install_status(app)
            app_status_layout = [sg.Text(app.upper(), font=hdr_label_font, size=(12,0), pad=(0,5)),
                                 sg.Text(app_info['version'], font=hdr_value_font, size=(5,0), pad=(0,5)),
                                 sg.Text(install_status['summary'][1], font=hdr_value_font, text_color=install_status['summary'][2], size=(20,0))]
            if install_status['summary'][0]:
                app_key = f'-{app}-'
                app_key_list.append(app_key)        
                app_status_layout.append(sg.Button('Details', button_color=(install_status['summary'][2]), key=app_key))
            
            app_layout.append(app_status_layout)
            app_status[app] = install_status
        layout = [
                   [sg.Text("Select 'Details' for the complete installation status.\n", font=hdr_value_font)],
                   app_layout,
                   [sg.Text('', font=hdr_label_font)],
                   [sg.Button('Done', button_color=('gray'), key=f'-DONE-')]
                 ]

        window = sg.Window(f'App Target Status', layout, resizable=True, finalize=True)
        while True: # Event Loop
            self.event, self.values = window.read()
            if self.event in (sg.WIN_CLOSED, '-DONE-') or self.event is None:       
                break
            if self.event in app_key_list:
                app = self.event.strip('-')
                target_status    = app_status[app]['target']
                topic_ids_status = app_status[app]['topic_ids']
                detailed_status = (
                    f"App in {self.cmake_app_list}?\n"
                    f"   {target_status.app_in_applist['state']}, {target_status.app_in_applist['descr']}\n"
                    f"Tables in {self.cmake_file_list}?\n"
                    f"   {target_status.tbl_in_filelist['state']}, {target_status.tbl_in_filelist['descr']}\n"
                    f"Table files in basecamp_defs?\n"
                    f"   {target_status.tbl_files_in_defs['state']}, {target_status.tbl_files_in_defs['descr']}\n"
                    f"App in {CFE_STARTUP_SCR}?\n"
                    f"   {target_status.app_in_startup_scr['state']}, {target_status.app_in_startup_scr['descr']}\n"
                    f"App dependencies in {self.cmake_app_list}?\n"
                    f"   {target_status.depend_in_applist['state']}, {target_status.depend_in_applist['descr']}\n"
                    f"App dependencies in {CFE_STARTUP_SCR}?\n"
                    f"   {target_status.depend_in_startup_scr['state']}, {target_status.depend_in_startup_scr['descr']}\n"
                    f"App ini table topic IDs in {self.cfe_topic_id_filename}?\n"
                    f"   {topic_ids_status.ini_topics_defined['state']}, {topic_ids_status.ini_topics_defined['descr']}\n"
                    f"App telemetry topic IDs in KIT_TO filter table?\n"
                    f"   {topic_ids_status.topics_in_kit_to['state']}, {topic_ids_status.topics_in_kit_to['descr']}\n"
                    )
                sg.popup(detailed_status, title=f"{app.upper()} Detailed Status", font=('Courier',11), line_width=150, keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)

        window.close()
        
    def add_usr_app_list(self, usr_app_list):
        """
        This is used to add a list of apps to a target without using a GUI
        """
        self.manage_usr_apps = ManageUsrApps(self.usr_app_path)
        status_text = ''
        for app in usr_app_list:
            status_text += self.add_usr_app(app, quiet_ops=True)
        return status_text
        
    def execute(self, action):
        self.manage_usr_apps = ManageUsrApps(self.usr_app_path)
        self.cfs_app_specs = self.manage_usr_apps.get_app_specs()
        if len(self.cfs_app_specs) > 0:
            usr_app_list = list(self.cfs_app_specs.keys())
            if action == 'Add':
                self.add_usr_app_gui(usr_app_list)
            elif action == 'Remove':
                self.remove_usr_app_gui(usr_app_list)
            elif action == 'App':
                self.usr_app_status_gui(usr_app_list)
        else:
            sg.popup('Your usr/apps directory is empty', title=f'{action} App', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)

    def get_app_table_list(self):
        app_cmake_files = self.usr_app_spec.get_targets_cmake_files()
        table_list_str = ""
        table_list = []
        for table in app_cmake_files['tables']:
            table_list_str += "%s, " % table
            table_list.append(table)
        return table_list
                
    def build_target(self):
        build_cfs_script = os.path.join(self.basecamp_abs_path, Cfs.SH_BUILD_CFS_TOPICIDS)
        build_cfs_target(build_cfs_script, self.cfs_abs_base_path, self.main_window)
                
    def restart_popup(self, instructions):
        """
        return sg.popup_ok_cancel(f"Wait for the 'Built target mission-install' status line in the 'cFS Target Process Window' that indicates the cFS target has been successfully built. Basecamp must be restarted to use the new command and telemetry definitions.\n\nSelect <OK> to shutdown Basecamp so it can be restarted.\n\n",
                                  title='Restart Basecamp GUI', font=('Arial', 14), keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
        """
        button = 'Cancel'
        text_width = 80
        layout = [[sg.Text("Wait for the 'Built target mission-install' status line in the 'cFS Target Process Window' before restarting Basecamp.", size=(text_width, None), auto_size_text=True)],
                  [sg.Text(f'\n{instructions}\n', text_color='red', size=(text_width, None), auto_size_text=True)],
                  [sg.Text("\nSelect <OK> to shutdown Basecamp so it can be restarted.")],
                  [sg.Ok(button_color=('SpringGreen4'), pad=(2,1)), sg.Cancel(button_color=('gray'), pad=(2,1))]]

        window = sg.Window('Restart Basecamp GUI', layout, grab_anywhere=True, modal=True)

        while True:  # Event Loop
            pop_event, pop_values = window.read(timeout=100)
            if pop_event in (sg.WIN_CLOSED, 'Cancel'):
                break
            elif pop_event == 'Ok':
                button = pop_event
                break

        window.close()
        return button
        
    def restart_main_gui(self, instructions):
        """
        This performs the same functionality as the '-4_AUTO-' window event. It is provided for 
        non-GUI situations. 
        """
        restart = False
        button = self.restart_popup(instructions)
        if button == 'Ok':
            restart = True        
            self.main_window['-RESTART-'].click()
        return restart

    def copy_app_tables(self, cfe_obj_type, app_framework, auto_copy):
        """
        An app's JSON spec table filename should not have a target prefix. The
        default table filename in an app's tables directory should have a 
        default target name. 
        
        There may be extra table files in an app's table directory so only copy
        the tables that are defined in the JSON app spec.
        """
        copy_passed = True
        popup_text = 'Undefined'
        table_list = self.get_app_table_list()
        table_count = len(table_list)
        if app_framework == AppSpec.APP_FRAMEWORK_CFS:
            popup_text = "No JSON tables copied since it's a cFS app with binary tables"
        else:
            if table_count == 0:
                if cfe_obj_type == AppSpec.CFE_TYPE_LIB:
                    popup_text = "No tables copied since it's a library"
                else:
                    popup_text = "Error in Basecamp app spec, no JSON tables listed."
                    copy_passed = False
            else:
                app_table_path = os.path.join(self.usr_app_path, self.selected_app, 'fsw', 'tables')
                if auto_copy:
                    target_equals_default = (DEFAULT_TARGET_NAME == self.cfs_target)
                    try:
                        src=''   # Init for exception
                        dst=''
                        target_prefix = DEFAULT_TARGET_NAME+'_'
                        tables_copied = 0
                        for table in os.listdir(app_table_path):
                            print(f'### {self.selected_app} table: {table}')
                            src_table = table.replace(target_prefix,'')
                            if src_table in table_list:
                                src = os.path.join(app_table_path, table)
                                #print('##src: ' + src)
                                if target_equals_default:
                                    dst_table = table
                                else:
                                    dst_table = self.cfs_target + '_' + src_table
                                dst = os.path.join(self.cfs_abs_defs_path, dst_table)
                                #print('##dst: ' + dst)
                                shutil.copyfile(src, dst)
                                tables_copied += 1
                        if tables_copied == table_count:
                            popup_text = f"Copied table files '{table_list}'\n\nFROM {app_table_path}\n\nTO {self.cfs_abs_defs_path}\n"
                        else:
                            popup_text = f"Failed to copy {table_count-tables_copied} file(s) from'{table_list}'. Source file(s) not found.\n"
                    except IOError:
                        popup_text = f'Error copying table file\nFROM\n  {src}\nTO\n  {dst}\n'
                else:
                    popup_text = f"Copy table files '{table_list}'\n\nFROM {app_table_path}\n\nTO {self.cfs_abs_defs_path}\n"
                    sg.popup(popup_text, title='Copy table files', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
        return copy_passed, popup_text
        
    def remove_app_tables(self):
        table_list = self.get_app_table_list()
        for table in table_list:
            try:
                table_file = os.path.join(self.cfs_abs_defs_path, f'{self.cfs_target}_{table}')
                os.remove(table_file)
                logger.info(f'Successfully removed {table_file} from {self.cfs_abs_defs_path}')
            except Exception as e:
                logger.error(f'Attempt to remove {table_file} raised exception: {repr(e)} ')
                
    def update_targets_cmake(self, auto_update):
        """
        The following two variables list need to be updated:
           SET(cpu1_APPLIST app1 app2 app3) #!BASECAMP-INSERT!
           SET(cpu1_FILELIST file1 file2) #!BASECAMP-INSERT!
        This logic assumes there is only one uncommented APPLIST and
        FILELIST line that needs to be updated. 
        """
        update_passed = True
        popup_text = "Undefined"
        app_cmake_files = self.usr_app_spec.get_targets_cmake_files()
        table_list_str = ''
        table_list = app_cmake_files['tables']
        if len(table_list) > 0:
            table_list_str = f'and {str(table_list)}'
        if auto_update:
            file_modified = False
            instantiated_text = ''
            with open(self.targets_cmake_file) as f:
                for line in f:
                    (line_modified, newline) = self.update_targets_cmake_line(app_cmake_files, line)
                    instantiated_text += newline
                    if line_modified:
                        file_modified = True
            
            if file_modified:
                with open(self.targets_cmake_file, 'w') as f:
                    f.write(instantiated_text)
                popup_text = f"Updated {self.targets_cmake_file} with {app_cmake_files['obj-file']} {table_list_str}"
            else:
                popup_text = f"Preserved {self.targets_cmake_file}, it already contains {app_cmake_files['obj-file']} {table_list_str}"
            #todo: Remove? sg.popup(popup_text, title=f'Update {self.targets_cmake_filename}', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
        else:
            sg.clipboard_set(app_cmake_files['obj-file'] + ',' + str(app_cmake_files['tables']))
            popup_text = f"After this dialogue, {self.targets_cmake_filename} will open in an editor. Paste\n  {app_cmake_files['obj-file']}\ninto\n  {self.cmake_app_list}\n\nPaste filenames with spaces\n  {app_cmake_files['tables']}\ninto\n  {self.cmake_file_list}"
            sg.popup(popup_text, title=f'Update {self.targets_cmake_filename}', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
            self.text_editor = sg.execute_py_file('texteditor.py', parms=self.targets_cmake_file, cwd=self.basecamp_tools_path)
        return update_passed, popup_text
        
    def update_targets_cmake_line(self, app_cmake_files, line):
        line_modified = False
        if INSERT_KEYWORD in line:
            if self.cmake_app_list in line:
                # The string search logic looks for an exact match and allows
                # the name being searched to exist in an end-of-line comment
                if not line.strip().startswith('#'):  # Non-commented line
                    i = line.find(')')
                    if not app_cmake_files['obj-file'] in line[:i].split(' '):
                        line = line[:i] + ' ' + app_cmake_files['obj-file'] + line[i:]     
                        line_modified = True
                        print('app_list_new: ' + line)
            elif self.cmake_file_list in line:
                if not line.strip().startswith('#'):  # Non-commented line
                    for table in app_cmake_files['tables']:
                        i = line.find(')')
                        if not table in line[:i].split(' '):
                            line = line[:i] + ' ' + table + line[i:]     
                            line_modified = True
                            print('file_list_new: ' + line)
        return (line_modified, line)
        
    def restore_targets_cmake(self):
        app_cmake_files = self.usr_app_spec.get_targets_cmake_files()
        obj_file   = app_cmake_files['obj-file'] 
        table_list = self.get_app_table_list()
        
        file_modified = False
        instantiated_text = ''
        with open(self.targets_cmake_file) as f:
            for line in f:
                if INSERT_KEYWORD in line:
                    if self.cmake_app_list in line:
                        # The string search logic looks for an exact match and allows
                        # the name being searched to exist in an end-of-line comment
                        if not line.strip().startswith('#'):  # Non-commented line
                            i = line.find(')')
                            obj_file_list = line[:i].split(' ')
                            try:
                                index = obj_file_list.index(obj_file)
                                del obj_file_list[index]
                                logger.info(f'Removed {obj_file} from {line} in file {self.targets_cmake_file}')
                                line = " ".join(obj_file_list) + line[i:]     
                                file_modified = True
                            except ValueError:
                                logger.info(f'Attempt to remove {obj_file} from {self.targets_cmake_file}, but not in app list: {line}')   
                    elif self.cmake_file_list in line:
                        for table in table_list:
                            if table in line:
                                line = line.replace(table,"")
                                file_modified = True
                                logger.info(f'Removed {table} from {self.targets_cmake_file}')
                instantiated_text += line        
        if file_modified:
            with open(self.targets_cmake_file, 'w') as f:
                f.write(instantiated_text)

    def update_startup_scr(self, auto_update):
        update_passed = True
        startup_script_entry = self.usr_app_spec.get_startup_scr_entry()
        if auto_update:
            original_entry = ""
            check_for_entry = True
            file_modified = False
            instantiated_text = ""
            with open(self.startup_scr_file) as f:
                for line in f:
                    if check_for_entry:
                        line_array = [field.strip() for field in line.split(',')]
                        if self.selected_app in line_array:
                            check_for_entry = False
                            original_entry = line
                        if INSERT_KEYWORD in line:
                            # If check_for_entry still true then entry hasn't been found
                            if check_for_entry:
                                line = startup_script_entry+'\n'+line
                                file_modified = True
                            check_for_entry = False
                    instantiated_text += line               
            if file_modified:
                with open(self.startup_scr_file, 'w') as f:
                    f.write(instantiated_text)
                popup_text = f'Added {self.selected_app} to startup script entry'
            else:
                popup_text = f'Preserved startup script, it already contains {self.selected_app}'
            #todo: Remove? sg.popup(popup_text, title=f'Update {self.startup_scr_filename}', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
        else:
            sg.clipboard_set(startup_script_entry)
            popup_text = f"After this dialogue, {self.startup_scr_filename} will open in an editor.\nPaste the following entry from the clipboard:\n\n'{startup_script_entry}'\n"
            sg.popup(popup_text, title=f'Update {self.startup_scr_filename}', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
            self.text_editor = sg.execute_py_file('texteditor.py', parms=self.startup_scr_file, cwd=self.basecamp_tools_path)
        return update_passed, popup_text
    
    def restore_startup_scr(self):
        """
        Search for the app's entry. Any field with app's name can be used as
        a keyword for the search
        """
        startup_script_entry = self.usr_app_spec.get_startup_scr_entry()
        keyword = startup_script_entry.split(',')[2]
        check_for_entry = True
        file_modified   = False
        instantiated_text = ""
        with open(self.startup_scr_file) as f:
            for line in f:
                if check_for_entry:
                    if keyword in line:
                        line = ''
                        check_for_entry = False
                        file_modified   = True
                    if INSERT_KEYWORD in line:
                        check_for_entry = False
                instantiated_text += line               
        if file_modified:
            with open(self.startup_scr_file, 'w') as f:
                f.write(instantiated_text)

    def update_topic_ids(self):
        """
        cfe-topicids.xml and kit_to_pkt_tbl.json are updated together because
        they both use the telemetry topic IDs. Also with the current 'spare'
        ID substitution method, this code makes sure they both have enough
        spares before either is updated.
        """
        update_passed = False
        popup_text = 'Undefined'
        cmd_topics = self.usr_app_spec.get_cmd_topics()
        tlm_topics = self.usr_app_spec.get_tlm_topics()
        cfe_topic_ids  = CfeTopicIds(self.cfe_topic_id_file)
        kit_to_pkt_tbl = JsonTblTopicMap(self.kit_to_tbl_file)
        
        if len(cmd_topics) > cfe_topic_ids.spare_cmd_topic_cnt():
            popup_text = f'Error acquiring command topic IDs. {len(cmd_topics)} needed, only {cfe_topic_ids.spare_cmd_topic_cnt()} available.'
            sg.popup(popup_text, title='Update Topic IDs Error', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
        elif len(tlm_topics) > cfe_topic_ids.spare_tlm_topic_cnt():
            popup_text = f'Error acquiring cFE telemetry topic IDs. {len(tlm_topics)} needed, only {cfe_topic_ids.spare_tlm_topic_cnt()} available.'
            sg.popup(popup_text, title='Update Topic IDs Error', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
        elif len(tlm_topics) > len(kit_to_pkt_tbl.spare_topics()):
            popup_text = f'Error acquiring KIT_TO telemetry topic IDs. {len(tlm_topics)} needed, only {len(kit_to_pkt_tbl.spare_topics())} available.'
            sg.popup(popup_text, title='Update Topic IDs Error', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
        else:  
            for cmd in cmd_topics:
                cfe_topic_ids.replace_spare_cmd_topic(cmd)
                #print(cmd)
            for tlm in tlm_topics:
                cfe_topic_ids.replace_spare_tlm_topic(tlm)
                #print(tlm)
            cfe_topic_ids.write_doc_to_file()
            kit_to_pkt_tbl.replace_spare_topics(tlm_topics)
            popup_text = f'Updated topic IDs in {self.cfe_topic_id_file} and {self.kit_to_tbl_file}'
            update_passed = True
            #todo: Remove? sg.popup(popup_text, title='Update Topic IDs', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
        return update_passed, popup_text 
        
    def restore_topic_ids(self):
        if self.usr_app_spec.app_name not in RESERVED_APPS:
            cmd_topics = self.usr_app_spec.get_cmd_topics()
            tlm_topics = self.usr_app_spec.get_tlm_topics()
            cfe_topic_ids  = CfeTopicIds(self.cfe_topic_id_file)
            kit_to_pkt_tbl = JsonTblTopicMap(self.kit_to_tbl_file)
            for cmd in cmd_topics:
                cfe_topic_ids.restore_spare_cmd_topic(cmd)
            for tlm in tlm_topics:
                cfe_topic_ids.restore_spare_tlm_topic(tlm)
            cfe_topic_ids.write_doc_to_file()
            kit_to_pkt_tbl.restore_spare_topics(tlm_topics)
            
    def remove_app_src_files(self):
        app_path = os.path.join(self.usr_app_path, self.selected_app)
        try:
           shutil.rmtree(app_path)
           logger.info(f'Successfully removed {app_path}')
        except Exception as e:
           logger.error(f'Attempt to remove {app_path} raised exception: {repr(e)} ')
           
    def app_install_status(self, app):
        """
        A properly installed app includes:
          1. App defined in targets.cmake APP_LIST
          2. Tables defined in target.cmakes and present in basecamp_defs folder
          3. App defined in startup script 
          4. Dependencies exist in APP_LIST and startup script
          5. Topic IDs defined in ini file are defined in cfe-topicids.xml
          6. Telemetry topic IDs defined in TO's table
        """
        app_target_status = self.verify_app_installed_in_target(app)
        app_topic_ids = self.verify_app_topic_ids(app)
        #app_target_status.print()
        #app_topic_ids.print()
        
        summary_status = [True,  f'Incomplete Installation', 'IndianRed3']
        if app_target_status.all_true():
            # kit_to is not consider an app installation error. It's status is still reported
            if app_topic_ids.ini_topics_defined['state']:
                summary_status = [True,  f'Valid Installation', 'SpringGreen4']
        elif app_target_status.app_not_installed():
            summary_status = [False, f'Not Installed', 'gray']

        app_status = { 'summary': summary_status, 'target': app_target_status, 'topic_ids': app_topic_ids}
        return app_status
 
    def in_targets_cmake_list(self, line, cpu_list, item):
        """
        Searches for 'item' in the cpu_list(e.g.cpu1_APPLIST). The string
        search logic looks for an exact match and handles situations
        when keywords or items being search are in comments
        """
        item_in_list = self.TRI_STATE.NOT_APP
        if INSERT_KEYWORD in line:
            if cpu_list in line:
               if not line.strip().startswith('#'):  # Non-commented line
                    i = line.find(')')
                    if item in line[:i].split(' '):
                        item_in_list = self.TRI_STATE.TRUE
                    else:
                        # app_c_fw may be in the global applist
                        if item == 'app_c_fw':
                            item_in_list = self.TRI_STATE.TRUE
                        else:
                            item_in_list = self.TRI_STATE.FALSE
        return item_in_list

    def verify_dependencies_installed(self, app):
        """
        Verify an app's dependencies defined by JSON 'requires' are installed.
        Dependencies include libraries and other apps. App dependencies may
        have timing relationships that are not satisfied simply by the app
        loading order so this check does not guarantee proper operation. 
        """
        self.usr_app_spec = self.manage_usr_apps.get_app_spec(app)
        app_info = self.usr_app_spec.get_app_info()
        
        req_libs_installed = True
        missing_req_libs = []
        for req_lib in app_info['requires']:
            print(f'req_lib: {req_lib}')
            if 'app_c_fw' not in req_lib:
                app_target_status = self.verify_app_installed_in_target(req_lib)
                app_target_status.print()
                if not app_target_status.all_true():
                    req_libs_installed = False
                    missing_req_libs.append(req_lib)
        return req_libs_installed, missing_req_libs
        
    def verify_app_installed_in_target(self, app):
        """
        Verifies:
          1. App defined in targets.cmake APP_LIST
          2. Tables defined in targets.cmake and present in basecamp_defs folder
          3. App defined in startup script 
          4. Dependencies exist in APP_LIST and startup script
        """
        # Status defaults to valid and each verification below sets invalid states
        target_status = AppTargetStatus()

        try:
            self.usr_app_spec = self.manage_usr_apps.get_app_spec(app)
        except:
            target_status.invalidate_all()
            return target_status
            
        app_info          = self.usr_app_spec.get_app_info()
        app_cmake_files   = self.usr_app_spec.get_targets_cmake_files()
        app_table_list    = self.get_app_table_list()

        # 1. Verify targets.cmake
        
        with open(self.targets_cmake_file) as f:
            for line in f:
                # INSERT_KEYWORD check is in_targets_cmake_list(). Check here for efficiency  
                if INSERT_KEYWORD in line:
                    if self.in_targets_cmake_list(line, self.cmake_app_list, app) == self.TRI_STATE.FALSE:
                        target_status.app_in_applist['state'] = False
                        target_status.app_in_applist['descr'] = f"{app} not in {self.targets_cmake_filename}'s {self.cmake_app_list}"
                    
                    if any(self.in_targets_cmake_list(line, self.cmake_app_list, req_app) == self.TRI_STATE.FALSE for req_app in app_info['requires']):
                        target_status.depend_in_applist['state'] = False
                        target_status.depend_in_applist['descr'] = f"Not all dependencies {app_info['requires']} found in {self.targets_cmake_filename}'s {self.cmake_app_list}"

                    if any(self.in_targets_cmake_list(line, self.cmake_file_list, req_tbl) == self.TRI_STATE.FALSE for req_tbl in app_cmake_files['tables']):
                        target_status.tbl_in_filelist['state'] = False
                        target_status.tbl_in_filelist['descr'] = f"Not all tables {app_cmake_files['tables']} found in {self.targets_cmake_filename}'s {self.cmake_file_list}"

        # 2. Verify CFE_STARTUP_SCR
        
        # Create list of startup script apps 
        startup_app_list = []
        with open(self.startup_scr_file) as f:
            for line in f:
                if line.strip().startswith('!'):
                    break
                line_array = [field.strip() for field in line.split(',')]
                if len(line_array) > 1:
                    startup_app_list.append(line_array[1])  # App object name is second field
         
        if not app in startup_app_list:
            target_status.app_in_startup_scr['state'] = False
            target_status.app_in_startup_scr['descr'] = f'{app} not in {self.startup_scr_filename}'
               
        if not all(req_app in startup_app_list for req_app in app_info['requires']):
            target_status.depend_in_startup_scr['state'] = False
            target_status.depend_in_startup_scr['descr'] = f"Not all dependencies {app_info['requires']} found in {self.startup_scr_filename}"
        
        # 3. Verify table files in basecamp_defs
        
        defs_file_list = os.listdir(self.cfs_abs_defs_path)

        if not all(f'{self.cfs_target}_{req_tbl}' in defs_file_list for req_tbl in app_table_list):
            # OSK tables must be copied into basecamp_defs, cFS tables are generated during build process
            if app_info['framework'] == AppSpec.APP_FRAMEWORK_OSK:
                target_status.tbl_files_in_defs['state'] = False
                target_status.tbl_files_in_defs['descr'] = f"Not all tables {app_table_list} found in basecamp_defs directory"
            elif app_info['framework'] == AppSpec.APP_FRAMEWORK_CFS:
                target_status.tbl_files_in_defs['descr'] = f"cFS apps do not require tables in basecamp_defs directory"
                
        return target_status
         
    def verify_app_topic_ids(self, app):
        """
        Verifies:
          1. Topic IDs defined in ini file are defined in cfe-topicids.xml
          2. Telemetry topic IDs defined in TO's table
        """
        self.usr_app_spec = self.manage_usr_apps.get_app_spec(app)
        app_info          = self.usr_app_spec.get_app_info()
        app_table_list    = self.get_app_table_list()

        # Status defaults to valid and each verification below sets invalid states
        topic_id_status = AppTopicIdStatus()

        startup_script_entry = self.usr_app_spec.get_startup_scr_entry()
        print(f'startup_script_entry: {startup_script_entry}')
        if startup_script_entry.split(',')[0].strip() == 'CFE_LIB':
            return topic_id_status
        
        # 1. Verify ini table topic IDs are defined in cfe-topicids.xml
        
        init_table = [tbl for tbl in app_table_list if "_ini" in tbl]
        init_table_len = len(init_table)
        if init_table_len == 1:
            ini_filename = os.path.join(self.cfs_abs_defs_path,f'{self.cfs_target}_{init_table[0]}')
            ini_topic_ids = []
            try:
                with open(ini_filename) as f:
                    for line in f:
                        if ':' in line:
                            keyword = line.split(':')
                            keyword_str = keyword[0].strip().strip('"')
                            if '_TOPICID' in keyword_str:
                                ini_topic_ids.append(keyword_str)
                with open(self.cfe_topic_id_file) as f:
                    cfe_topic_ids = f.read()
                if not all(topic_id in cfe_topic_ids for topic_id in ini_topic_ids): # Simple check not concerned with commented out topics
                    if app_info['framework'] == AppSpec.APP_FRAMEWORK_OSK:
                        topic_id_status.ini_topics_defined['state'] = False
                        topic_id_status.ini_topics_defined['descr'] = f"Not all ini table topic IDs {ini_topic_ids} found in {self.cfe_topic_id_filename}"            
                    elif app_info['framework'] == AppSpec.APP_FRAMEWORK_CFS:
                        topic_id_status.ini_topics_defined['descr'] = f"cFS apps do not require topic IDs in a JOSN ini file"
               
                # 2. Verify kit_to filter table contains ini table telemetry topic IDs. This isn't required but good practice for testing               
                with open(self.kit_to_tbl_file) as f:
                    kit_to_tbl = f.read()
                ini_tlm_topic_ids = [topic_id for topic_id in ini_topic_ids if "TLM_TOPICID" in topic_id]
                if not all(topic_id in kit_to_tbl for topic_id in ini_tlm_topic_ids): # Simple check not concerned with commented out topics
                    topic_id_status.topics_in_kit_to['state'] = False
                    topic_id_status.topics_in_kit_to['descr'] = f"Not all ini table tlm topic IDs {ini_tlm_topic_ids} found in {self.kit_to_tbl_filename}"            
            except IOError:
                topic_id_status.ini_topics_defined['state'] = False
                topic_id_status.ini_topics_defined['descr'] = f"App's ini table file {init_table[0]} not in basecamp_defs directory"            
            
        elif init_table_len == 0:
            if app_info['framework'] == AppSpec.APP_FRAMEWORK_OSK:
                topic_id_status.ini_topics_defined['state'] = False
                topic_id_status.ini_topics_defined['descr'] = f"Init table not found in {app_table_list} using '_ini' substring"
            elif app_info['framework'] == AppSpec.APP_FRAMEWORK_CFS:
                topic_id_status.ini_topics_defined['descr'] = f"cFS apps do not require a JOSN '_ini' file"             
        else:
            topic_id_status.ini_topics_defined['state'] = False
            topic_id_status.ini_topics_defined['descr'] = f"Found more than 1 init table in {app_table_list} using '_ini' substring"
 
        return topic_id_status

