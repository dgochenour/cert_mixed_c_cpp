// (c) Copyright, Real-Time Innovations, 2021.  All rights reserved.
// RTI grants Licensee a license to use, modify, compile, and create derivative
// works of the software solely for use with RTI Connext DDS. Licensee may
// redistribute copies of the software provided that all such copies are subject
// to this license. The software is provided "as is", with no warranty of any
// type, including any warranty for fitness for any purpose. RTI is under no
// obligation to maintain or support the software. RTI shall not be liable for
// any incidental or consequential damages arising out of the use or inability
// to use the software.

#ifndef COMMON_CONFIG_H
#define COMMON_CONFIG_H

#include <iostream>

auto domain_id = 100;

// discovery-related constants for example_publisher
static const std::string k_publisher_initial_peer   = "127.0.0.1";
static const std::string k_PARTICIPANT01_NAME       = "publisher";
static const int k_OBJ_ID_PARTICIPANT01_DW01        = 100;

// discovery-related constants for example_subscriber 
static const std::string k_subscriber_initial_peer  = "127.0.0.1";
static const std::string k_PARTICIPANT02_NAME       = "subscriber";
static const int k_OBJ_ID_PARTICIPANT02_DR01        = 200;

#endif
