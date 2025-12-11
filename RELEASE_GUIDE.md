# How to Release a New Update for SimplePresenter

Follow these steps whenever you want to release a new version (e.g., going from `1.0.0` to `1.0.1`).

## 1. Update Version Numbers

Before building, you must update the version number in the code.

1.  **Open `CMakeLists.txt`**:
    *   Find the line: `project(SimplePresenter VERSION 1.0.0 LANGUAGES CXX)`
    *   Change `1.0.0` to your new version (e.g., `1.0.1`).

2.  **Open `update.json`** (in the root folder):
    *   Change `"latest_version": "1.0.0"` to `"latest_version": "1.0.1"`.
    *   Update `"release_notes"` with a description of what changed.
    *   **Do not push this file to GitHub yet!** (We push it *after* the release is live).

---

## 2. Build for Windows

1.  **Open Qt Creator** (or Visual Studio).
2.  **Select Release Configuration**:
    *   Ensure you are building in **Release** mode (not Debug).
3.  **Build** the project.
    *   The output `SimplePresenter.exe` will be in your build folder (e.g., `build/Release/`).
4.  **Package the App**:
    *   Create a new folder named `SimplePresenter_v1.0.1_Windows`.
    *   Copy `SimplePresenter.exe` into it.
    *   Run the deployment tool to copy necessary Qt DLLs:
        Open PowerShell and run:
        ```powershell
        C:\Qt\6.10.1\mingw_64\bin\windeployqt.exe "path\to\SimplePresenter_v1.0.1_Windows\SimplePresenter.exe" --release --no-translations
        ```
    *   *Note: Adjust the path to `windeployqt.exe` based on your Qt installation.*
    *   Copy any extra data folders (like `data/`) or DLLs (like `ffmpeg` DLLs) if needed.
5.  **Zip the Folder**:
    *   Right-click `SimplePresenter_v1.0.1_Windows` -> Send to -> Compressed (zipped) folder.
    *   Name it `SimplePresenter_v1.0.1_Windows.zip`.

---

## 3. Build for macOS

1.  **Open Qt Creator** on your Mac.
2.  **Select Release Configuration**:
    *   Ensure you are building in **Release** mode.
3.  **Build** the project.
    *   This creates `SimplePresenter.app`.
4.  **Package the App (Create DMG)**:
    *   Open Terminal.
    *   Run `macdeployqt` to bundle frameworks and create a DMG:
        ```bash
        ~/Qt/6.10.1/macos/bin/macdeployqt path/to/SimplePresenter.app -dmg
        ```
    *   *Note: Adjust the path to `macdeployqt` based on your Qt installation.*
    *   This will generate `SimplePresenter.dmg` next to the `.app` file.
    *   Rename it to `SimplePresenter_v1.0.1_macOS.dmg`.

---

## 4. Publish Release on GitHub

1.  Go to: [https://github.com/psnact/SimplePresenter/releases](https://github.com/psnact/SimplePresenter/releases)
2.  Click **"Draft a new release"**.
3.  **Choose a tag**: Create a new tag (e.g., `v1.0.1`).
4.  **Release title**: `SimplePresenter v1.0.1`.
5.  **Description**: Paste your release notes here.
6.  **Attach binaries**:
    *   Drag and drop `SimplePresenter_v1.0.1_Windows.zip`.
    *   Drag and drop `SimplePresenter_v1.0.1_macOS.dmg`.
7.  Click **"Publish release"**.

---

## 5. Trigger the Update for Users

Now that the files are online, tell the app there is a new version.

1.  **Commit and Push `update.json`**:
    *   You updated this file in Step 1.
    *   Now commit it to the repository:
        ```powershell
        git add CMakeLists.txt update.json
        git commit -m "Release v1.0.1"
        git push
        ```

**Done!**
*   Users opening the app will now see "A new version (1.0.1) is available!" when they check for updates.
*   They will be directed to the GitHub release page you just created to download the new files.
