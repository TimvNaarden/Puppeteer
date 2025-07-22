### Puppeteer (Windows Only) 

This is a C++ project that I used to get a better understanding of C++ with components such as an UI(ImGUI) or Networking and communication with the Windows API.
Puppeteer is a project that consists of two programs, there is a client-side program(Puppet) this runs as a Windows service on the computer. 
The other program(Master) is a ImGUI application that connects to all the computers running the puppet service. 

## Puppeteer Master
The Master application allows you to watch multiple computers at at time, by displaying the screen contents of the connected puppets in a grid pattern. 
Furthermore, the application collects information about the Puppets internal components, such as CPU, GPU and Memory + Disksize. 
The application also allows you to disable the hardware input from a computer itself and take control of the machine. 

## Why? 
I made this program in my last High School year as an alternative to the Open Source Veyon, it was for me a way to learn the languege of C++. 
Our computer science class had a nework with around 60 computers, that each needed a way to be monitored. 

## Features
A grid overview of all the puppets and there screen contents, that gets refreshed every minute.
The option to take control over a puppet in a teamviewer kind of way, where you can also disable inputs from the user itself. 
A collection of internal data of the puppets, like GPU, CPU, and much more. This can also be exported to a external file.
A automatic update system for the Puppet program, to add new features or things like bug fixes.
Secured, all trafic is encrypted using OpenSSL and for connections to every puppet is an administrator account needed. 
An option to export and import the Master configuration accross multiple devices.

## Libraries 
I have written my own Networking and JSON data stroing libraries, so that I would get more experience with networking and data manipulation.
I am relying on the Windows API for the Puppet Service and authentication. The display monitoring is done by DirectX.
For the Master application is ImGUI being used for the graphical components with OpenGL. 

## Further Questions 
If you are interested in some of the details of this project can you reach my by email at TimvNaarden@outlook.com
