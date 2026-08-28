
1. Determine the required files  
    To record the API-calls from an application you first need to identify the *correct folder* for your endeavor by choosen the right API and OS and use it in the recording process.

   - On **Windows**, this is typically done by copying a DLL into the app directory.
   - On **Linux**, by manipulating loader environment variables.
   - When recording **Vulkan**, it is also possible to use **GITS** as a **Vulkan layer** instead. This method works on both OSs.

    !!! warning "Use layer or interceptor"
        Don't use both layer and interceptor when recording as it will lead to a corrupt and unusable GITS stream.

2. Configure the recording process  
   The file `gits_config.yml` configures GITS and the recording options. Details regarding the configuration options can be found in the documentation's [configuration section](/configuration/how_to.md).

3. Run the appliaption  
    Once the files are in place you simply start the application normally while GITS initiates the recording.
