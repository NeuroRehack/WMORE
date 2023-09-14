"""
File: backup_to_drive.py
Description: This script backs up the data folder to Google Drive.
Author: Sami Kaab
Date: 2023-07-05
"""

import os
import os.path
import shutil
import logging
import requests
from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build
from googleapiclient.errors import HttpError
from googleapiclient.http import MediaFileUpload
from google.oauth2 import service_account


# Constants
SCOPES = ["https://www.googleapis.com/auth/drive"]
FOLDER_NAME = "WMORE_DATA"
DATA_DIR = "data"
BACKUP_DIR = "BackedUp"


def setup_logging():
    logfilename = '/root/WMORE/logs/WMORE.log'
    # create file if it doesn't exist
    if not os.path.exists(logfilename):
        with open(logfilename, 'w') as f:
            pass
    #setup logging  
    logging.basicConfig(filename=logfilename, level=logging.INFO, format='%(asctime)s %(levelname)s %(name)s %(message)s')


def get_credentials():
    """
    Get the user credentials for Google Drive.

    Returns:
        The user credentials for Google Drive.
    """
    # Check if a token file exists and load the credentials from it if it does
    creds = None
    creds = service_account.Credentials.from_service_account_file("credentials.json", scopes=SCOPES)

    return creds


def is_internet_available():
    """Check if an internet connection is available."""
    try:
        requests.get("http://google.com", timeout=5)
        # logging.info("Internet access available")
        return True
    except:
        # logging.error("No internet access available")
        return False

def delete_all():
    # Set up the service account credentials
    credentials = get_credentials()

    # Build the Google Drive API client
    drive_service = build("drive", "v3", credentials=credentials)

    # List all files
    response = drive_service.files().list().execute()
    files = response.get("files", [])

    # Iterate through the files and delete them
    for file in files:
        file_id = file["id"]
        file_name = file["name"]
        drive_service.files().delete(fileId=file_id).execute()
        print(f"Deleted file: {file_name}")
        
def download_all():
    credentials = get_credentials()
   # Build the Google Drive API client
    drive_service = build("drive", "v3", credentials=credentials)

    # Retrieve file metadata with parent information
    response = drive_service.files().list(fields="files(id, name, mimeType, parents)").execute()
    files = response.get("files", [])

    # Iterate through the files and download them
    for file in files:
        file_id = file["id"]
        file_name = file["name"]
        parents = file.get("parents", [])
        mime_type = file["mimeType"]

        # Create the folder structure on the local system
        folder_path = ""
        for parent_id in parents:
            parent_response = drive_service.files().get(fileId=parent_id, fields="name").execute()
            parent_name = parent_response.get("name", "")
            folder_path = os.path.join(parent_name, folder_path)
            os.makedirs(folder_path, exist_ok=True)

        # Download the file
        if "application/vnd.google-apps" in mime_type:
            print(f"Skipping export for file: {file_name}. It is not a supported Google Docs Editors file.")
            continue

        request = drive_service.files().get_media(fileId=file_id)
        file_content = request.execute()

        file_path = os.path.join(folder_path, file_name)
        with open(file_path, "wb") as f:
            f.write(file_content)

        print(f"Downloaded file: {file_path}")




def create_folder(service, name, parent_folder_id=None):
    # check if folder exists and return id if it does
    query = f"name='{name}' and mimeType='application/vnd.google-apps.folder'"
    if parent_folder_id:
        # if parent_folder_id is specified, add it to the query
        query += f" and '{parent_folder_id}' in parents"

    response = service.files().list(q=query, spaces='drive', fields='files(id)').execute()
    if len(response['files']) > 0:
        return response['files'][0]['id']
    
    
    folder_metadata = {
        'name': name,
        'mimeType': 'application/vnd.google-apps.folder'
    }
    if parent_folder_id:
        folder_metadata['parents'] = [parent_folder_id]

    folder = service.files().create(body=folder_metadata, fields='id').execute()
    return folder['id']

def upload_file(service, file_path, parent_folder_id):
    # check if file exists and return id if it does
    query = f"name='{os.path.basename(file_path)}' and '{parent_folder_id}' in parents"
    response = service.files().list(q=query, spaces='drive', fields='files(id)').execute()
    if len(response['files']) > 0:
        return response['files'][0]['id']
    
    file_metadata = {
        'name': os.path.basename(file_path),
        'parents': [parent_folder_id]
    }

    media = MediaFileUpload(file_path, mimetype='application/octet-stream')
    file = service.files().create(body=file_metadata, media_body=media, fields='id').execute()
    logging.info(f"Backed up file: {file_path}")
    return file['id']



def clone_folder_structure(service, local_folder, parent_folder_id=None):
    
    folder_name = os.path.basename(local_folder)
    folder_id = create_folder(service, folder_name, parent_folder_id)

    for item in os.listdir(local_folder):
        item_path = os.path.join(local_folder, item)
        if os.path.isdir(item_path):
            clone_folder_structure(service, item_path, folder_id)
        else:
            upload_file(service, item_path, folder_id)

def upload():
    # Get the user credentials for Google Drive
    creds = get_credentials()
    # Build the Google Drive API client with the user credentials
    service = build("drive", "v3", credentials=creds)
    local_root_folder = DATA_DIR
    drive_root_folder_id = create_folder(service, FOLDER_NAME)  # Replace 'My Drive' with your desired top-level folder name
    clone_folder_structure(service,local_root_folder, drive_root_folder_id)

if __name__ == "__main__":
#     # setup_logging()
#     # os.makedirs(BACKUP_DIR, exist_ok=True)
    
    # delete_all()
    # exit()

#     if is_internet_available():
#         file_list = []
#         # recursively walk the directory tree and add file paths to the list
#         for root, dirs, files in os.walk(DATA_DIR):
#             for filename in files:
#                 file_list.append(os.path.join(root, filename))       
#         print(file_list)
#         backup_files(file_list)
#         # download_all()
#         # get_credentials()
    upload()