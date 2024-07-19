# Airborne-CPS
A Traffic Collision Avoidance System Mark II (TCAS-II) implementation for Laminar Research X-Plane

### Current Functionality

- Threat detection within a protection volume.
- Threat classifications:
  * Traffic Advisory(TA)
  * Resolution Advisory(RA)
- RA action consensus and action suggestion.
- Drawing of recommended action(vertical velocity) to gauge.
- Suport for at least 3 client connections.
- Strength-Based RA Selection Algorithm for 3+ aircraft RA scenarios.

### Description
This project is a plugin for the Laminar Research X-Plane flight simulator, and is an implementation of a [Traffic Collision Avoidance System](https://www.faa.gov/documentLibrary/media/Advisory_Circular/TCAS%20II%20V7.1%20Intro%20booklet.pdf), mark II. It is used in Cyber Physical Systems research at the State University of New York at Oswego; please see "Literature" for examples and related work.

- The plugin was written in C++ using Microsoft Visual Studio Community 2022.
- The SDK for the plugin is included in the codebase, and can also be found [here](http://www.xsquawkbox.net/xpsdk/mediawiki/Main_Page).

# Setup

## Authors

Robert Sgroi (rsgroi@oswego.edu)

Logan Nguyen (nnguyen6@oswego.edu 

Bastian Tenbergen (bastian.tenbergen@oswego.edu)

Principal Investigator: 

Prof. Bastian Tenbergen (bastian.tenbergen@oswego.edu)

*Department of Computer Science*

*State University of New York at Oswego, USA*

## Intorduction 

This Guide explains how to get TCAS plugin deployed and running on any (and only on) windows machines. It goes through the steps of downloading and installing all applications involved in the deployment of the TCAS Project.

Quick note: If you work on the computers in the lab room in the Shinemen building (professor Tenbergen will give you a tour on it), then there are high chances that the Airborne-CPS in those computers are not up to date. Simply follow this guide to update them. 

## Whaat you will you need

* X-Plane Flight Simulator (X-plane 10)
* Visual Studio Community (2015,2019,2022)

## Downloading and Installing X-Plane 10

You can Download CD A: [HERE](http://cs.oswego.edu/~tenberge/cpslab/xp10dvds/). 

1. Double Click on CD A to open it, and double click on the Installer_windows.exe.
2. Choose a directory to download X-Plane to and save the address for later use. 
3. Accept the User Agreement.
5. The next window will allow the User to pick which portions of the world they would like installed in the simulator. Feel Free to download the entire world if you’d like, however that is a 60GB download. The only portion of the Map we use for testing is the area down below.

![alt text][image1]

Fig. 1: Area Tiles Needed For Airborne CPS TCAS plugin

6. After selecting the terrain you want download the discs X-plane asks for [HERE](http://cs.oswego.edu/~tenberge/cpslab/xp10dvds/).
7. once downloaded open up the disc by double clicking it then go back to the install wizard and continue the install process.

After the X-plane 10 install is complete and you can move on to Visual Studio Code

## Downloading and Installing Visual Studio (2015, 2019 0r 2022)

The Download link for Visual Studio can be found [HERE](https://visualstudio.microsoft.com/downloads/). This link will bring you to a downloads page where Visual Studio can be found.

If you wish to download versions older than 2022, scroll the end of the page, and click the [Older Downloads >](https://visualstudio.microsoft.com/vs/older-downloads/)

Be sure to select the following Visual Studio components:

* Desktop Development with C++ 
* Git for Windows

Once installation is complete, move on to cloning the Airborne-CPS repository.

## The Airborne-CPS GitHub Repository

1. The GitHub repo is publicly located at https://github.com/tenbergen/Airborne-CPS (upstream repo)
2. Be sure to fork the upstream repo to your own GitHub account (original repo) to make yourself familiar with the project. Once you’re ready to make contributions, work on your own form and submit Pull Requests to the upstream repo.
3. Now, go to your local machine and navigate to the location you wish to store the repo. Then, either use git to clone the repo (git clone "yourForkedRepoGitLink”) or just simply download the zip folder from GitHub. The folder will have the same name as the GitHub repo’s, "Airborne-CPS".
4. When you got the Airborne-CPS folder cloned or unzipped on the local machine, again for safety purpose, you should create a new git branch (git checkout -b yourBranch) to work on so you won't accidentally mess up the work in master branch.

### Configuring Visual Studio

Now, let's open the project in Visual Studio. In this guide, we’re using Visual Studio 2022 to demonstrate the process. If you used older VS versions, it should not be too different from VS 2022. 

1. Open your Visual Studio and you should find something like this window below:

![alt text][image2]

Fig. 2: Visual Studio 2022 Get Started Page

2. Click the "Open a local folder" then navigate to the location you stored the Airborne-CPS folder. Open it! Now you get this,

![alt text][image3]

Fig. 3: Visual Studio 2022 Project Home with Airborne-CPS

3. In the Solution Explorer (left side), click on "Airborne-CPS.sln"
4. If Visual Studio asks to install additional components, install them. Depending on your installation, “Desktop Development with C++” might be required.
5. Then click and expand the "Airborne-CPS (Visual Studio 2019)", you should see the all the .cpp and .h files listed in the Solution Explorer panel.
6. In your taskbar, next to the left of “Local Windows Debugger” you should see two dropdown boxes which state “Debug” and “x64”. Change the “Debug” to “Release” and the “x64” to “x86”.
7. Now, the real configurations will start from here. Make sure that you select the "Airborne-CPS (Visual Studio 2019)" in Solution Explorer so that it's highlighted, then from the very top part of the VS idea, click on Project -> Properties (you might see “Properties for Airborne-CPS”). This will open the Properties window of the entire project

![alt text][image4]

Fig. 4: Visual Studio 2022 Airborne-CPS Properties Page

8. Now, we need to change the Output Directory of the Project. This is the folder where the build of the plug-in will be placed once we run our program. Within the "Airborne-CPS Property page" (the screenshot above), make sure you're on the "Configuration Properties" tab and “General”, you'll need to change the "Output Directory".
9. Click on the "Output Directory", now click on the dropdown button (very end of the line) and then press Browse. From here, you'll need to navigate to the X-Plane 10 Folder we downloaded before. Within the X-Plane Folder you need to navigate to Resources, and then plugins (X-Plane 10\Resources\plugins). This is the Output Directory for the Project when we build the project in Visual Studio.  

![alt text][image5]

Fig. 5: Visual Studio 2022 Airborne-CPS Configuration Properties

10. Next we need to move to the C/C++ tab on the left side of the screen, then navigate to **Additional Include Directories**.

![alt text][image6]

Fig. 6: Visual Studio 2022 Airborne-CPS Additional Include Directory Configuration

11. From here you will need to click on the Drop-Down arrow that appears on the right side of the **Additional Include Directories** tab, then press <Edit>. This will open the Additional Include Directories Directory Manager. 
12. If you see paths beginning with “$(SolutionDir)”, keep them and skip to step 15).
13. Else, you might see some auto-generated paths/directories here. You can click on the those and use the RED X in the top right to delete them all. 
14. Next to the left of the RED X button is a yellow folder and a star, this will allow you to include a new Directory. Once you press on this, you will notice that a new Blank Directory Tab was created, and on the right side of that tab is a button showing “…”. Click on that button to browse your computer. Now, after pressing on the “…” button, you must navigate to the “Airborne-CPS” folder we cloned earlier. The first directory we will include will be **(Airborne-CPS\src)**. While in the **Airborne-CPS** folder, open the src folder, then press “Select Folder”. This should now add **(…\Airborne-CPS\src)** to your Additional Include Directories Manager. You will repeat this process five more times, navigating to different folders within the Airborne-CPS-master folder. The Additional Include Libraries you will need to include

*Airborne-CPS\src (We just included this one)
*Airborne-CPS\SDK\CHeaders
*Airborne-CPS\SDK\CHeaders\XPLM
*Airborne-CPS\SDK\CHeaders\Widgets
*Airborne-CPS\SDK\Delphi\XPLM
*Airborne-CPS\SDK\Delphi\Widgets

![alt text][image7]
Fig. 7: Visual Studio 2022 Airborne-CPS Included Directories from the Repository

15. When those are all added make sure to press “Ok” and the click “Apply”. 
16. Next, we will have to add the “Additional Library Directories” in the Linker. On the left side of the Properties menu, there is a dropdown button named Linker, press this and open up the General tab. There will be a tab labeled “Additional Library Directories”

![alt text][image8]
Fig. 8: Visual Studio 2022 Airborne-CPS Additional Library Directories

17. This will work the same as the “Additional Include Directories” we added before (step 13). Just click on the Additional Library Directories, press the dropdown button, and press “<Edit>”. From here you will remove whatever directories showing and then add two more. These Three Directories are:
* Airborne-CPS\SDK\Libraries\Win
* Airborne-CPS\Release\32
18. Now, you might not see the Release folder in the Airborne-CPS folder. Don't panic! Just ignore it for now, keep moving on with the next steps below. At the end, after you build the project for the first time, you will see it and you can add it at that moment.
19. Once these are added, you can press “OK” and then hit “Apply”.

![alt text][image10]

Fig 9 Visual Studio 2022 Airborne-CPS Post build events

20. Then open the "build events" then click "post-build events"
21. You should see a row called command line with a xcopy command, if you dont see this ignore the rest of the steps
22.  Change the xcopy command from xcopy "$(SolutionDir)out/x86" "c:\X-Plane 10\Resources\plugins\"  /Y to xcopy "$(SolutionDir)Release\32" "c:\X-Plane 10\Resources\plugins\"  /Y
23.  You can then hit okay and apply and close the project window

###Folder Navigation

Navigate to the Airborne-CPS folder cloned earlier, then move to the Specification Directory. Copy the “images” folder that’s there and then open up a new File Explorer window and navigate to the X-Plane 10 folder. Go to X-Plane 10\Resources\plugins and paste the “images” folder we copied earlier, rename that “images” folder to AirborneCPS. Head back to the Airborne-CPS folder and within the same directory the “images” folder was in, you will find the “situations” folder *(…\Airborne-CPS\Specifications)*.  Copy that situations folder, and head back over to the X-Plane 10 Folder. Within the X-Plane 10 folder, look for **X-Plane 10\Output**, and replace the “situation” folder there with the one we just copied. You can now exit out of all the File explorers and head back over to Visual Studio Community so we can finally Build the Plugin.

###Building the plugin 

Now, make sure that X-plane 10 is NOT running, then in Visual Studio, click the **Build** tab on the top of the screen, and press **“Clean Airborne-CPS”**. Then you can click the **Build** tab again and press **“Build Airborne-CPS”** (or Ctrl+b), this will build the project and place it in the X-Plane 10 Plugin folder as we directed it to. You might see something like this below,
![alt text][image9]
Fig. 10: Visual Studio 2022 Airborne-CPS Successful Built Process

From here, you can open X-Plane 10, use the Cessna 172SP, and begin! If all steps were followed correctly, you should see the TCAS Gauge operating on the bottom right of the screen.
If you have any questions regarding deployment of Airborne-CPS contact the principal investigator.

## Troubleshooting
1. Some users have reported problems with loading X-plane after successfully compiling the plugin, if X-plane is crashing when trying to load AirbornCPS.xml or AirborneCPS.pdb then you can resolve this issue by deleting these files as the plugin will still work without these files.


[image1]: https://github.com/Tomicgun/Airborne-CPS-Thomas/blob/master/images%20for%20read%20me/X-plane%20Map.png
[image2]: https://github.com/Tomicgun/Airborne-CPS-Thomas/blob/master/images%20for%20read%20me/Visual%20Studio%20setup%201.png
[image3]: https://github.com/Tomicgun/Airborne-CPS-Thomas/blob/master/images%20for%20read%20me/Visual%20Studio%20setup%202.png
[image4]: https://github.com/Tomicgun/Airborne-CPS-Thomas/blob/master/images%20for%20read%20me/project%205.png
[image5]: https://github.com/Tomicgun/Airborne-CPS-Thomas/blob/master/images%20for%20read%20me/project%201.png
[image6]: https://github.com/Tomicgun/Airborne-CPS-Thomas/blob/master/images%20for%20read%20me/project%202.png
[image7]: https://github.com/Tomicgun/Airborne-CPS-Thomas/blob/master/images%20for%20read%20me/project%202%20b.png
[image8]: https://github.com/Tomicgun/Airborne-CPS-Thomas/blob/master/images%20for%20read%20me/project%203.png
[image9]: https://github.com/Tomicgun/Airborne-CPS-Thomas/blob/master/images%20for%20read%20me/fina.png
[image10]: https://github.com/Tomicgun/Airborne-CPS-Thomas/blob/master/images%20for%20read%20me/image10.jpg
