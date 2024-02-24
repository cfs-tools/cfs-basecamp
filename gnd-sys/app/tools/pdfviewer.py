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
        Provide a simple PDF file viewer
  
    Notes:
      1. See https://pymupdf.readthedocs.io/en/latest/index.html
         for pymupdf(fitz) module documentation
      2. Since the PDF documents being viewed are Basecamp docs
         that have similar styles, font sizes etc the viewer can
         be very simple without features like zooming.
          
"""
import sys
sys.path.append("..")
import fitz

from tools import PySimpleGUI_License
import PySimpleGUI as sg


###############################################################################

class PdfViewer():
    """
    """
    def __init__(self, filename):
          
        self.filename   = filename
        self.doc        = fitz.open(filename)
        self.page_count = len(self.doc)
        self.page_dlist = [None] * self.page_count # Page display list
        self.page_curr  = 0
        self.page_prev  = 0
 
 
    def get_page_png_image(self, page_num):

        if self.page_dlist[page_num] is None:
            self.page_dlist[page_num] = self.doc[page_num].get_displaylist()
        
        page      = self.page_dlist[page_num]        
        pixmap    = page.get_pixmap()
        png_image = pixmap.tobytes(output='png')
        
        return png_image


    def create_window(self):
    
        sg.theme('BluePurple')
        png_image = self.get_page_png_image(self.page_curr)

        layout = [
            [
                sg.Button('Prev'),
                sg.Button('Next'),
                sg.Text('Page'),
                sg.InputText(str(self.page_curr+1), size=(4,1), key='-PAGE_NUM-', justification='center'),
                sg.Text(f'of {self.page_count}')
            ],
            [   
                sg.Image(data=png_image, key='-PNG_IMAGE-')
            ]
        ]
        
        window = sg.Window(f'{self.filename}', layout, return_keyboard_events=True, use_default_focus=False, modal=True)
        return window        


    def execute(self):
    
        window = self.create_window()

        while True:
            event, values = window.read(timeout=100)
            if event in (sg.WIN_CLOSED, 'Exit') or event is None:
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
                png_image = self.get_page_png_image(self.page_curr)
                window['-PNG_IMAGE-'].update(png_image)
                window['-PAGE_NUM-'].update(str(self.page_curr+1))
                self.page_prev = self.page_curr
        
        window.close()
        

###############################################################################

if __name__ == '__main__':
    """
    sys.argv[0] - Name of script
    sys.argv[1] - If provided is the filename to be edited
    """
    filename = None
    if len(sys.argv) > 1:
        filename = sys.argv[1]
        #print ('filename = ' + filename)
        pdf_viewer = PdfViewer(filename)
        pdf_viewer.execute()
    else:
        print('Plese provide the PDF file name as the first argument')
