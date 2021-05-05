# Using the Connext Cert C API from a C++11 application

## Example Overview

This example contains publisher and subscriber applications for a simple IDL-defined data type. The purpose of this example is to demonstrate that the Connext Cert C-language API can be used from a C++11 (or newer) application. Such functionality may be interesting 
to developers eventually deploying safety-critical applications that use Connext Cert libraries.

This example in designed to link against libraries from the Connext DDS Micro 2.4.14 release built in a Cert-compatible configuration. With that in mind, a few choices were make the example code "Cert friendly."
* The available API in Connext Cert is based on C.  
* DPSE discovery is used as actual safety-certified libraries (ISO 26262, for example) implement the static DPSE discovery plugin only; DPDE is not available.

## Source Overview

A simple "example" type, containing a message string, is defined in `example.idl`.

For any user-defined type to be useable by Connext Micro or Connext Cert, type-support files must be generated that implement a type-plugin interface. The `rtiddsgen` tool automates this, and can be invoked manually, as shown below.

### Linux:

    $ $RTIMEHOME/rtiddsgen/scripts/rtiddsgen -micro -language C -create typefiles ./example.idl

### Windows:

    > %RTIMEHOME%\rtiddsgen\scripts\rtiddsgen -micro -language C -create typefiles example.idl


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
    ./resource/scripts/rtime-make --target Linux --name ${RTIMEARCH}_cert -G "Unix Makefiles" --build --config Release
    ./resource/scripts/rtime-make --target Linux --name ${RTIMEARCH}_cert -G "Unix Makefiles" --build --config Debug

## Building the Examples

### Linux

    cd your/project/directory 
    $ $RTIMEHOME/resource/scripts/rtime-make --config Release --build --name ${RTIMEARCH}_cert --target Linux --source-dir . -G "Unix Makefiles" --delete

The executables can be found in the ./objs/<architecture> directory

Note that there a few variables at the top of main() in both the publisher and
subscriber code that may need to be changed to match your system:

    // user-configurable values
    std::string peer = "127.0.0.1";
    std::string loopback_name = "lo0";
    std::string eth_nic_name = "en0";
    auto domain_id = 100;

By default, the remote peer is set to the loopback address (allowing the 
examples to discover each other on the same machine only) and DDS Domain 100 is 
used. The names of the network interfaces should match the actual host where the
example is running. 


## Running example_publisher and example_subscriber

Using a Linux system as the example, run the subscriber in one terminal with the 
command:

    $ objs/x64Linux4gcc7.3.0_cert/example_subscriber 

And run the publisher in another terminal with the command:

    $ objs/x64Linux4gcc7.3.0_cert/example_publisher 