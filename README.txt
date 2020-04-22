################################################################################
#
# RTI Connext DDS Micro 2.4.12
#
# EXAMPLE: Using the C API from a C++11 application
#
################################################################################

This application contains a publisher and subscriber of a simple IDL-defined 
data type. The purpose of this example is to demonstrate that Micro's C-language
API can be used from a C++11 application. Such functionality may be interesting 
when to developers who must use safety-certified Micro libraries, which only 
implement the C API, but would like to construct their wider user application in 
C++11 (or newer.)

Note that this example uses DPDE discovery even though actual safety-certified
libraries (ISO26262, for example) will only implement the static DPSE 
discovery plugin. This decision was made to simplify the example, and the same 
mixed C/C++11 use would be possible with DPSE instead of DPDE.


Source Overview
===============

A simple "example" type, containing a message string, is defined in 
example.idl.

For the type to be useable by Connext Micro, type-support files must be 
generated that implement a type-plugin interface.  The example Makefile 
will generate these support files, by invoking rtiddsgen.  Note that rtiddsgen
can be invoked manually, with an example command like this:

    $RTIMEHOME/rtiddsgen/scripts/rtiddsgen -micro -language C -update typefiles ./example.idl

The generated source files are example.c, exampleSupport.c, and 
examplePlugin.c. Associated header files are also generated.
 
The DataWriter and DataReader for this type are created and used in 
example_publisher.cxx and example_subscriber.cxx, respectively. Each 
application-- publisher and subscriber-- has its own DomainParticipant since 
the intent is that the executables may run independently of each other.


Generated Files Overview
========================

example_publisher.cxx:
This file contains the logic for creating a Publisher and a DataWriter, and 
sending data.  

example_subscriber.cxx:
This file contains the logic for creating a Subscriber and a DataReader, a 
DataReaderListener, and listening for data.

examplePlugin.c:
This file creates the plugin for the example data type.  This file contains 
the code for serializing and deserializing the example type, creating, 
copying, printing and deleting the example type, determining the size of the 
serialized type, and handling hashing a key, and creating the plug-in.

exampleSupport.c
This file defines the example type and its typed DataWriter, DataReader, and 
Sequence.

example.c
This file contains the APIs for managing the example type. 


Compiling w/ cmake
==================

The following assumptions are made:

-   The environment variable RTIMEHOME is set to the Connext Micro installation 
    directory. 
-   Micro libraries exist in your installation for the architecture in question. 
    If you are unsure if this is the case, please consult the product 
    documentation.
    https://community.rti.com/static/documentation/connext-micro/2.4.12/doc/html/usersmanual/index.html

On MacOS (Darwin) 
-----------------
    $ cd your/project/directory 
    $ $RTIMEHOME/resource/scripts/rtime-make --config Release --build --name x64Darwin17.3.0Clang9.0.0 --target Darwin --source-dir . -G "Unix Makefiles" --delete

On Linux
--------
    $ cd your/project/directory 
    $ $RTIMEHOME/resource/scripts/rtime-make --config Release --build --name x64Linux3gcc4.8.2 --target Linux --source-dir . -G "Unix Makefiles" --delete

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


Running example_publisher and example_subscriber
================================================

Using a Linux system as the example, run the subscriber in one terminal with the 
command:

    $ objs/i86Linux2.6gcc4.4.5/example_subscriber 

And run the publisher in another terminal with the command:

    $ objs/i86Linux2.6gcc4.4.5/example_publisher 
