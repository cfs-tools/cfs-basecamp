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
        Provide classes to manage tutorials and lessons within a tutorial.

    Notes:
      1. JSON key constants should all be used within the Json class so if any key
         changes only the Json class will change.
      2. Issue #21 https://github.com/cfs-tools/cfs-basecamp/issues/51 changed
         tutorial image format from PNG files to a single PDF file. The
         original tutorial and lesson classes were renamed to TutorialPng and
         LessonPng and preserved as an available options. 

"""

import sys
import time
import os
import json
import configparser
import fitz
from datetime import datetime

import logging
logger = logging.getLogger(__name__)

if __name__ == '__main__':
    from jsonfile import JsonFile
    from utils    import compress_abs_path
else:
    from .jsonfile import JsonFile
    from .utils    import compress_abs_path

import PySimpleGUI as sg


TUTORIAL_JSON_FILE = 'tutorial.json'
LESSON_JSON_FILE   = 'lesson.json'
LESSON_DIR         = 'lesson'

###############################################################################

class TutorialJson(JsonFile):
    """
    Manage tutor and lesson JSON files. Keep tag names and formats consistent.
    The tutor and lesson JSON files have nearly idenical keys and since this is
    not a generic utility this one class takes care of both file types.
    """
    def __init__(self, json_file):
        super().__init__(json_file)
        
    def lesson_slide(self):
        return self.json['current-slide']

    def lesson_complete(self):
        return self.json['complete']

    def reset_child(self):
        if 'current-slide' in self.json:
           self.json['current-slide'] = 1
           self.json['complete'] = False

    def set_complete(self, complete):
        self.json['complete'] = complete

    def set_slide(self, slide):
        self.json['current-slide'] = slide
    
    
###############################################################################

class Lesson():
    """
    Manage the display for a lesson. The lesson's JSON file is used to 
    determine the initial state. The execute() method allows a lesson to be
    restarted and override the JSON. The new lesson state is recorded in the
    JSON when the lesson is exited.
    """
    def __init__(self, number, path, pdf_file):

        self.number    = number
        self.path      = path
        self.pdf_file  = pdf_file
        self.load_json()

        self.doc        = fitz.open(os.path.join(self.path,self.pdf_file))
        self.page_count = len(self.doc)
        self.page_dlist = [None] * self.page_count # Page display list
        self.page_curr  = 0
        self.page_prev  = 0


    def get_pdf_page_png_image(self, page_num):

        if self.page_dlist[page_num] is None:
            self.page_dlist[page_num] = self.doc[page_num].get_displaylist()
        
        page      = self.page_dlist[page_num]        
        pixmap    = page.get_pixmap()
        png_image = pixmap.tobytes(output='png')
        return png_image
        
    def load_json(self):
    
        self.json = TutorialJson(os.path.join(self.path, LESSON_JSON_FILE))
        
        self.title     = self.json.title()
        self.cur_slide = self.json.lesson_slide()
        self.complete  = self.json.lesson_complete()

    def reset(self):
        self.json.reset()
        self.load_json()
        
    def create_window(self):
    
        png_image = self.get_pdf_page_png_image(self.page_curr)

        layout = [
            [
                sg.Button('Prev'), sg.Button('Next'),
                sg.Text('Page'), sg.InputText(str(self.page_curr+1), size=(4,1), key='-PAGE_NUM-', justification='center'), sg.Text(f'of {self.page_count}'),
                sg.Button('Mark as Complete', pad=(10,0))
            ],
            [   
                sg.Image(data=png_image, key='-PNG_IMAGE-')
            ]
        ]
        
        window = sg.Window(f'{self.pdf_file}', layout, return_keyboard_events=True, use_default_focus=False, resizable=True, modal=True)
        return window        


    def execute(self):
    
        window = self.create_window()

        while True:
            event, values = window.read(timeout=100)
            if event in (sg.WIN_CLOSED, 'Exit') or event is None:
                break

            if event == 'Mark as Complete':
               self.json.set_complete(True)
               break
               
            if event in ("Next", "MouseWheel:Down", "Down:116", "Next:117", "KP_Next:89"):
                self.page_curr += 1
                if self.page_curr >= self.page_count:
                    self.page_curr = 0                
            elif event in ("Prev", "MouseWheel:Up", "Up:111", "Prior:112", "KP_Prior:81"):
                self.page_curr -= 1
                if self.page_curr < 0:
                    self.page_curr = self.page_count-1                

            if self.page_curr != self.page_prev:
                png_image = self.get_pdf_page_png_image(self.page_curr)
                window['-PNG_IMAGE-'].update(png_image)
                window['-PAGE_NUM-'].update(str(self.page_curr+1))
                self.page_prev = self.page_curr

        self.json.set_slide(self.page_curr)
        self.json.update()
        window.close()       
        return self.json.lesson_complete()

###############################################################################

class Tutorial():
    """
    Manage the display for a tutorial. A tutorial folder contains a
    tutorial.json that describes the tutorial and a lesson folder that 
    contains numbered lesson folders. Each lesson folder contains a 
    lesson.json file.  
    """
    def __init__(self, tutorial_path):

        self.path = tutorial_path
        self.json = TutorialJson(os.path.join(tutorial_path, TUTORIAL_JSON_FILE))
        
        self.lesson_path = os.path.join(tutorial_path,LESSON_DIR)
        logger.debug(f'self.lesson_path  = {self.lesson_path}')
        self.lesson_list = [int(l) for l in os.listdir(self.lesson_path) if l.isnumeric()]
        self.lesson_list.sort()
        logger.debug(f'self.lesson_list = {str(self.lesson_list)}')
        
        self.display = True
        self.reset   = False


    def create_lesson_objs(self):
        self.lesson_objs = {}
        for l in self.lesson_list:
            lesson_num_path = os.path.join(self.lesson_path, str(l))
            logger.debug(f'lesson_num_path = {lesson_num_path}')
            # Use first PDF file found
            lesson_pdf_file = None
            for file in os.listdir(lesson_num_path):
                if file.lower().endswith('.pdf'):
                    lesson_pdf_file = file
                    break
            if lesson_pdf_file is not None:
                logger.debug(f'Lesson {l} pdf file = {str(lesson_pdf_file)}')
                self.lesson_objs[l] = Lesson(l, lesson_num_path, lesson_pdf_file)            
            else:
                logger.debug(f'No PDF found for lesson {l}')
    
    def create_window(self):
        """
        Create the main window. Non-class variables are used so it can be refreshed, PySimpleGui
        layouts can't be shared.
        """
        hdr_label_font = ('Arial bold',12)
        hdr_value_font = ('Arial',12)
        
        objective_text = ""
        for objective_line in self.json.objective():
            objective_text += objective_line

        resume_lesson = 1
        for lesson in self.lesson_objs.values():
            if lesson.complete:
                resume_lesson += 1
            else:
                break
                    
        lesson_layout = []
        for lesson in self.lesson_objs.values():
            logger.debug("Lesson Layout " + lesson.title)
            title          = f'{lesson.number}-{lesson.title}'
            complete_state = 'Yes' if lesson.complete else 'No'
            radio_state    = True  if lesson.number == resume_lesson else False
            lesson_layout.append([sg.Radio(title, 'LESSONS', default=radio_state, font=hdr_value_font, size=(30,0), key=f'-LESSON{lesson.number}-'), sg.Text(complete_state, key=f'-COMPLETE{lesson.number}-')])
        
        
        # Layouts can't be reused/shared so if someone does a tutorial reset it causes issues if layout is a class variable
        layout = [
                  [sg.Text('Objectives', font=hdr_label_font)],
                  [sg.MLine(default_text=objective_text, font = hdr_value_font, size=(40, 4))],
                  # Lesson size less than lesson layout so complete status will appear centered 
                  [sg.Text('Lesson', font=hdr_label_font, size=(28,0)),sg.Text('Complete', font=hdr_label_font, size=(10,0))],  
                  lesson_layout, 
                  [sg.Button('Start', button_color=('SpringGreen4')), sg.Button('Reset'), sg.Button('Exit')]
                 ]

        window = sg.Window(self.json.title(), layout, modal=True)
        return window
        
    def selected_lesson(self):
        """
        Return the selected lesson.
        """
        lesson_obj = None
        for lesson in self.lesson_objs:
            if self.values[f'-LESSON{lesson}-'] == True:
               lesson_obj = self.lesson_objs[lesson]
               break
               
        return lesson_obj

        
    def gui(self):
        """
        Navigating through lessons is not strictly enforced.  The goal is to keep the user
        interface very simple so the algorithm to determine which lesson to resume is simplistic
        and it's up to the user whether they select lessons as completed.
        """
                
        while self.display:
          
            self.create_lesson_objs()
            window = self.create_window()

            while True: # Event Loop

                self.event, self.values = window.read(timeout=100)
                   
                if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:       
                    break
            
                if self.event == 'Start':
                    lesson_obj = self.selected_lesson()
                    if lesson_obj is not None: 
                        if lesson_obj.execute():
                            window[f'-COMPLETE{lesson_obj.number}-'].update('Yes') 
                            # If not last lesson then redisplay lesson window
                            if lesson_obj.number < len(self.lesson_objs):
                                self.reset = True
                            break

                if self.event == 'Reset':
                    for lesson in list(self.lesson_objs.values()):
                       lesson.reset()   
                    self.reset = True
                    break
        
            self.json.update()
            window.close()
        
            if self.reset:
                self.reset = False
            else:
                self.display = False
        
    def execute(self):
        self.gui()


###############################################################################

class LessonPng():
    """
    Manage the display for a lesson. The lesson's JSON file is used to 
    determine the initial state. The execute() method allows a lesson to be
    restarted and override the JSON. The new lesson state is recorded in the
    JSON when the lesson is exited.
    """
    def __init__(self, number, path, slides):

        self.number    = number
        self.path      = path
        self.slides    = slides
        self.slide_max = len(slides)
        self.load_json()

    def load_json(self):
    
        self.json = TutorialJson(os.path.join(self.path, LESSON_JSON_FILE))
        
        self.title     = self.json.title()
        self.cur_slide = self.json.lesson_slide()
        self.complete  = self.json.lesson_complete()

        
    def update_slide_file(self):
        self.slide_file = os.path.join(self.path, self.slides[self.cur_slide-1])
    
    
    def gui(self):
        
        self.update_slide_file()
        
        layout = [
                  [sg.Text('Slide'), sg.Text(str(self.cur_slide), pad=(2,1), key='-SLIDE-')],   
                  [sg.Image(filename=self.slide_file, key='-IMAGE-')],
                  [sg.Button('Prev', size=(8,2)), sg.Button('Next', size=(8, 2)), sg.Button('Mark as Complete', pad=(10,0))]
                 ]
 
        self.window = sg.Window(self.json.title(), layout, element_justification='c', resizable=True, modal=True)
        
        while True:

            self.event, self.values = self.window.read()
        
            if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:
                break

            if self.event == 'Mark as Complete':
               self.json.set_complete(True)
               break
               
            if self.event == 'Prev':
                if self.cur_slide > 1:
                    self.cur_slide -= 1
            
            elif self.event == 'Next':
                if self.cur_slide < self.slide_max:
                    self.cur_slide += 1
            
            self.update_slide_file()
            logger.debug(f'filename = {self.slide_file}')
            
            self.window['-SLIDE-'].update(str(self.cur_slide))
            self.window['-IMAGE-'].update(filename=self.slide_file)

        self.json.set_slide(self.cur_slide)
        self.json.update()
        self.window.close()       

        
    def execute(self):
        self.gui()
        return self.json.lesson_complete()
        
    def reset(self):
        self.json.reset()
        self.load_json()


###############################################################################

class TutorialPng():
    """
    Manage the display for a tutorial. A tutorial folder contains a
    tutorial.json that describes the tutorial and a lesson folder that 
    contains numbered lesson folders. Each lesson folder contains a 
    lesson.json file.  
    """
    def __init__(self, tutorial_path):

        self.path = tutorial_path
        self.json = TutorialJson(os.path.join(tutorial_path, TUTORIAL_JSON_FILE))
        
        self.lesson_path = os.path.join(tutorial_path,LESSON_DIR)
        logger.debug(f'self.lesson_path  = {self.lesson_path}')
        self.lesson_list = [int(l) for l in os.listdir(self.lesson_path) if l.isnumeric()]
        self.lesson_list.sort()
        logger.debug(f'self.lesson_list = {str(self.lesson_list)}')
        self.lesson_objs = {}
        for l in self.lesson_list:
            lesson_num_path = os.path.join(self.lesson_path, str(l))
            logger.debug(f'lesson_num_path = {lesson_num_path}')
            lesson_pngs = [f for f in os.listdir(lesson_num_path) if f.lower().endswith('.png')]
            lesson_pngs.sort()
            logger.debug(f'lesson_pngs = {str(lesson_pngs)}')
            self.lesson_objs[l] = LessonPng(l, lesson_num_path, lesson_pngs)
        
        self.display = True
        self.reset   = False

    def create_window(self):
        """
        Create the main window. Non-class variables are used so it can be refreshed, PySimpleGui
        layouts can't be shared.
        """
        hdr_label_font = ('Arial bold',12)
        hdr_value_font = ('Arial',12)
        
        objective_text = ""
        for objective_line in self.json.objective():
            objective_text += objective_line

        resume_lesson = 1
        for lesson in self.lesson_objs.values():
            if lesson.complete:
                resume_lesson += 1
            else:
                break
                    
        lesson_layout = []
        for lesson in self.lesson_objs.values():
            logger.debug("Lesson Layout " + lesson.title)
            title          = f'{lesson.number}-{lesson.title}'
            complete_state = 'Yes' if lesson.complete else 'No'
            radio_state    = True if lesson.number == resume_lesson else False
            lesson_layout.append([sg.Radio(title, 'LESSONS', default=radio_state, font=hdr_value_font, size=(30,0), key=f'-LESSON{lesson.number}-'), sg.Text(complete_state, key=f'-COMPLETE{lesson.number}-')])
        
        
        # Layouts can't be reused/shared so if someone does a tutorial reset it causes issues if layout is a class variable
        layout = [
                  [sg.Text('Objectives', font=hdr_label_font)],
                  [sg.MLine(default_text=objective_text, font = hdr_value_font, size=(40, 4))],
                  # Lesson size less than lesson layout so complete status will appear centered 
                  [sg.Text('Lesson', font=hdr_label_font, size=(28,0)),sg.Text('Complete', font=hdr_label_font, size=(10,0))],  
                  lesson_layout, 
                  [sg.Button('Start', button_color=('SpringGreen4')), sg.Button('Reset'), sg.Button('Exit')]
                 ]

        window = sg.Window(self.json.title(), layout, modal=True)
        return window
        
        
    def gui(self):
        """
        Navigating through lessons is not strictly enforced.  The goal is to keep the user
        interface very simple so the algorithm to determine which lesson to resume is simplistic
        and it's up to the user whether they select lessons as completed.
        """
                
        while self.display:

            window = self.create_window()

            while True: # Event Loop

                self.event, self.values = window.read(timeout=100)
                   
                if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:       
                    break
            
                if self.event == 'Start':
                    for lesson in self.lesson_objs:
                        if self.values[f'-LESSON{lesson}-'] == True:
                            if self.lesson_objs[lesson].execute():
                                window[f'-COMPLETE{lesson}-'].update('Yes') 
                
                if self.event == 'Reset':
                    for lesson in list(self.lesson_objs.values()):
                       lesson.reset()   
                    self.reset = True
                    break
        
            self.json.update()
            window.close()
        
            if self.reset:
                self.reset = False
            else:
                self.display = False
        
    def execute(self):
        self.gui()


###############################################################################

class ManageTutorials():
    """
    Discover what tutorials exists (each tutorial in separate directory) and
    create a 'database' of information about the tutorials based on each 
    tutorial's JSON spec.
    User select tutorials based by title so self.tutorial_lookup provides a
    method to retrieve a tutorial given its title
    """
    def __init__(self, tutorials_path):

        self.path = tutorials_path
        self.tutorial_titles = []
        self.tutorial_lookup = {}  # [title]  => Tutorial
        
        tutorial_list = os.listdir(tutorials_path)
        tutorial_list.sort()
        for tutorial_folder in tutorial_list:
            logger.debug(f'Tutorial folder: {tutorial_folder}')
            #todo: Tutorial constructor could raise exception if JSON doesn't exist or is malformed
            tutorial_json_file = os.path.join(tutorials_path, tutorial_folder, TUTORIAL_JSON_FILE)
            if os.path.exists(tutorial_json_file):
                #tutorial = TutorialPng(os.path.join(tutorials_path, tutorial_folder))
                tutorial = Tutorial(os.path.join(tutorials_path, tutorial_folder))
                self.tutorial_titles.append(tutorial.json.title())
                self.tutorial_lookup[tutorial.json.title()] = tutorial
        
        logger.debug(f'Tutorial Titles {str(self.tutorial_titles)}')
        logger.debug(f'Tutorial Lookup {str(self.tutorial_lookup)}')

    def run_tutorial(self, tutorial_title):
        if tutorial_title in self.tutorial_titles:
            self.tutorial_lookup[tutorial_title].execute()


###############################################################################

if __name__ == '__main__':

    tutorial_rel_dir = None
    if len(sys.argv) > 1:
        tutorial_rel_dir = sys.argv[1]
    else:
        tutorial_rel_dir = '1-basecamp-intro'
        
    config = configparser.ConfigParser()
    config.read('../basecamp.ini')
    TUTORIALS_PATH = config.get('PATHS','TUTORIALS_PATH')

    tutorial_dir = compress_abs_path(os.path.join(os.getcwd(),'..', TUTORIALS_PATH, tutorial_rel_dir)) 
    print ("tutorial_dir = " + tutorial_dir)
    tutorial = Tutorial(tutorial_dir)
    #tutorial = TutorialPng(tutorial_dir)
    tutorial.execute()
    
    

