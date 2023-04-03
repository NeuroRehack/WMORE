"""
File: ConversionGUI.py
Author: Sami Kaab
Date: March 16, 2023
Description:    This program converts every .bin file in a given directory to .csv format and merges the resulting .csv files.
                The script uses a PyQt5 graphical user interface to provide a user-friendly experience. The conversion function is
                run in a multiprocessing pool to speed up the process, and a progress bar is displayed to show the conversion progress.
                The merged .csv file is written to the selected directory, and the script provides an option to open the directory or
                exit the program.
to do: rework merge functionnality
"""

import os
import sys
from PySide2.QtWidgets import QApplication, QMainWindow, QFileDialog, QPushButton, QLabel, QVBoxLayout, QHBoxLayout, QWidget, QMessageBox, QProgressDialog
from PySide2.QtCore import Qt, QThread, Signal, QObject
from PySide2.QtGui import QIcon

import BinToCSV
import multiprocessing
import glob
import pandas as pd
import threading
import time
import queue

from constants import *

class ConvertWindow(QMainWindow):
    def __init__(self,app):
        super().__init__()
        self.setWindowTitle("Data Conversion")
        icon = QIcon(ICON_PATH)
        self.setWindowIcon(icon)

        self.app = app
        self.directory = ""
        self.csv_files = []

        # Create a label to show selected directory
        self.directory_label = QLabel(self)
        self.directory_label.setText('No directory selected')

        # Create a button to select directory
        self.select_button = QPushButton('Select Directory', self)
        self.select_button.clicked.connect(self.select_directory)

        # Create a button to run conversion
        self.convert_button = QPushButton('Convert', self)
        self.convert_button.clicked.connect(self.convert)
        self.convert_button.setEnabled(False)
        
        self.open_dir_button = QPushButton('Open Directory', self)
        self.open_dir_button.clicked.connect(lambda: os.startfile(self.directory))
        self.open_dir_button.setEnabled(False)
        
        # # Enable the merge button
        # self.merge_button = QPushButton('Merge CSV files', self)
        # self.merge_button.clicked.connect(self.merge_csv_files)
        # self.merge_button.setEnabled(False)
        
        # Set the central widget to a vertical layout containing the label and buttons
        central_widget = QWidget()
        layout = QVBoxLayout()
        layout.addWidget(self.directory_label)
        button_layout = QHBoxLayout()
        button_layout.addWidget(self.select_button)
        button_layout.addWidget(self.convert_button)
        button_layout.addWidget(self.open_dir_button)
        # button_layout.addWidget(self.merge_button)
        layout.addLayout(button_layout)
        central_widget.setLayout(layout)
        self.setCentralWidget(central_widget)

    def select_directory(self):
        # Open a file dialog to select directory
        directory = QFileDialog.getExistingDirectory(self, 'Select Directory')
        if directory is not None:
            self.directory = directory
        if os.path.exists(self.directory):
            self.open_dir_button.setEnabled(True)
        else:
            self.open_dir_button.setEnabled(False)
        # Update the directory label with the selected directory
        self.directory_label.setText(f'Selected directory: {self.directory}')

        # Check if there are ".bin" files in the selected directory
        if not any(file.endswith('.bin') for file in os.listdir(self.directory)):
            QMessageBox.warning(self, 'Warning', 'There are no ".bin" files in the selected directory.')
            self.convert_button.setEnabled(False)
        else:
            self.convert_button.setEnabled(True)
        # if not any(file.endswith('.csv') for file in os.listdir(self.directory)):
        #     self.merge_button.setEnabled(False)
        # else:
        #     self.merge_button.setEnabled(True)
            
            
    def convert(self):
        # Check if there are ".bin" files in the selected directory
        if not any(file.endswith('.bin') for file in os.listdir(self.directory)):
            QMessageBox.warning(self, 'Warning', 'There are no ".bin" files in the selected directory.')
            return

        extension = ".bin"
        files = glob.glob(os.path.join(self.directory, f"*{extension}"))
        # Set up the progress dialog
        num_files = len(files)
        progress = QProgressDialog("Converting files...", "Cancel", 0, num_files, self)
        progress.setWindowModality(Qt.WindowModal)
        progress.setAutoClose(True)
        progress.setAutoReset(True)
        progress.setWindowTitle("Converting files...")
        progress.show()

        # Run the conversion function in a multiprocess pool
        pool = multiprocessing.Pool()
        result = pool.map_async(BinToCSV.binToCSV, files)
        progress.setRange(num_files-result._number_left, num_files)
        while not result.ready():
            progress.setValue(num_files - result._number_left)
            self.app.processEvents()

        # Close the progress dialog when the conversion is finished
        progress.close()

        # Show a message box to inform the user that the conversion is complete
        QMessageBox.information(self, 'Information', 'Conversion complete.')
        # self.merge_button.setEnabled(True)
        
        
        
        # Store the paths of the CSV files generated by the conversion
        self.csv_files = [os.path.join(self.directory, f.replace(extension, ".csv")) for f in os.listdir(self.directory) if f.endswith(extension)]
        
    def merge_csv_files(self):
        # Get all CSV files in the selected directory
        csv_files = glob.glob(os.path.join(self.directory, '*.csv'))
        
        # Check if there are any CSV files to merge
        if not csv_files:
            QMessageBox.warning(self, 'Warning', 'There are no CSV files to merge.')
            return
        
        # Create a progress dialog for merging files
        self.progress = QProgressDialog("Merging files...", "Cancel", 0, len(csv_files), self)
        self.progress.setWindowModality(Qt.WindowModal)
        self.progress.setAutoClose(True)
        self.progress.setAutoReset(True)
        self.progress.setWindowTitle("Merging files...")
        self.progress.show()
        
        # Initialize a list to store data frames for each CSV file
        dfs = []
        
        # Read each CSV file and store its data frame in the list
        for csv_file in csv_files:
            df = pd.read_csv(csv_file)
            dfs.append(df)
            self.progress.setValue(self.progress.value() + 1)
            self.app.processEvents()
            
        
        # Concatenate all data frames in the list into a single data frame
        merged_df = pd.concat(dfs, ignore_index=True)
        # Write the merged data frame to a new CSV file
        output_path, _ = QFileDialog.getSaveFileName(self, 'Save Merged CSV File', '', 'CSV Files (*.csv)')
        if output_path:
            self.start_thread(merged_df,output_path)
        else:
            self.progress.close()
            QMessageBox.warning(self,'Information', 'Merged file not created!')
        
    def start_thread(self,merged_df,output_path):
        self.progress.setRange(0,0)
        self.worker_thread = writeMergedToCSV(merged_df,output_path)
        self.worker_thread.finished.connect(self.thread_finished)
        self.worker_thread.start()
        
    def thread_finished(self):
        self.progress.close()
        QMessageBox.information(self,'Information', 'CSV files merged successfully.')

        print("Thread finished")
       
        
        
class writeMergedToCSV(QThread):
    finished = Signal()

    def __init__(self,merged_df,output_path):
        super().__init__()
        self.merged_df = merged_df
        self.output_path = output_path

    def run(self):
        if self.output_path:
            self.merged_df.to_csv(self.output_path, index=False)
        self.finished.emit()  
    

        
if __name__ == '__main__':
    app = QApplication(sys.argv)

    window = ConvertWindow(app)
    window.show()

    sys.exit(app.exec_())
