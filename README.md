# Using the Connext Cert C API from a C++11 application

## Example Overview

This example contains publisher and subscriber applications for a simple IDL-defined data type. The purpose of this example is to demonstrate that the Connext Cert C-language API can be used from a C++11 (or newer) application. Such functionality may be interesting 
to developers eventually deploying safety-critical applications that use Connext Cert libraries.

This example in designed to link against libraries from the Connext DDS Micro 2.4.14 release built in a Cert-compatible configuration. With that in mind, a few choices were made to make the example code "Cert friendly."
* The available API in Connext Cert is based on C; this is why the DDS C API is used here, even though the application code is C++.  
* DPSE discovery is used because actual safety-certified libraries (ISO 26262, for example) implement the static DPSE discovery plugin only; DPDE is not available.

## Source Overview

A simple "example" type, containing a message string, is defined in `example.idl`.

For any user-defined type to be useable by Connext Micro or Connext Cert, type-support files must be generated that implement a type-plugin interface. The `rtiddsgen` tool automates this, and can be invoked manually, as shown below.

### Linux:

    $ $RTIMEHOME/rtiddsgen/scripts/rtiddsgen -micro -language C -create typefiles ./example.idl


The generated source files are example.c, exampleSupport.c, and 
examplePlugin.c. Associated header files are also generated. These source files are used by the middleware, and by the example_publisher.cxx and example_subscriber.cxx applications. Each application-- publisher and subscriber-- is wholely self-contained since the intent is that the executables may run independently of each other, and on different CPU/OS instances.


## Generated Files Overview

### `example_publisher.cxx`
This file contains the logic for creating a Publisher and a DataWriter, and sending data.  

### `example_subscriber.cxx`
This file contains the logic for creating a Subscriber and a DataReader, and receiving data.

### `examplePlugin.c`
This file creates the plugin for the example data type.  This file contains the code for serializing and deserializing the example type, creating, copying, printing and deleting the example type, determining the size of the serialized type, and handling hashing a key, and creating the plug-in.

### `exampleSupport.c`
This file implements the typed DataWriter, DataReader, and support functions to register/deregister the type. 

### `example.c` and `example.h`
These files contain the language-specific type implementation and the APIs for managing the type. 

## Building Cert-compatible Libraries

### Linux
    cd $RTIMEHOME
    ./resource/scripts/rtime-make --target Linux --name ${RTIMEARCH}_cert -G "Unix Makefiles" --build -DRTIME_CERT=1 --config Release 
    ./resource/scripts/rtime-make --target Linux --name ${RTIMEARCH}_cert -G "Unix Makefiles" --build -DRTIME_CERT=1 --config Debug

## Building the Examples

### Linux

    cd your/project/directory 
    $ $RTIMEHOME/resource/scripts/rtime-make --target Linux --name ${RTIMEARCH}_cert -G "Unix Makefiles" --build -DRTIME_CERT=1 --config Release --source-dir . --delete

The executables can be found in the ./objs/<architecture> directory

Note that different systems may have different interface names, and almost certainly will have different IP addresses. The file `common_config.h` contains interface configuration information that is used by both the publishing and subscribing applications: 

    // network interface information
    const std::string    k_loopback_name("loopback");
    const unsigned int   k_loopback_ip(0x7f000001);
    const unsigned int   k_loopback_mask(0xff000000);

    const std::string    k_real_nic_name("real_nic");
    const unsigned int   k_real_nic_ip(0xc0a80174);
    const unsigned int   k_real_nic_mask(0xffffff00);

`k_loopback_name` and `k_real_nic_name` need not match the *actual names* of the network interfaces as assigned by the OS. That is: on Ubunu 20.04 for example, it's OK to set `k_real_nic_name("real_nic")` instead of `k_real_nic_name("wlp0s20f3")`. By default, the remote peer is set to the loopback address (allowing the examples to discover each other on the same machine only) and DDS Domain 100 is 
used. 

## Running example_publisher and example_subscriber

Using a Linux system as the example, run the subscriber in one terminal with the 
command:

    $ objs/x64Linux4gcc7.3.0_cert/example_subscriber 

And run the publisher in another terminal with the command:

    $ objs/x64Linux4gcc7.3.0_cert/example_publisher 
