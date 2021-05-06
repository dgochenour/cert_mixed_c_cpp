// (c) Copyright, Real-Time Innovations, 2021.  All rights reserved.
// RTI grants Licensee a license to use, modify, compile, and create derivative
// works of the software solely for use with RTI Connext DDS. Licensee may
// redistribute copies of the software provided that all such copies are subject
// to this license. The software is provided "as is", with no warranty of any
// type, including any warranty for fitness for any purpose. RTI is under no
// obligation to maintain or support the software. RTI shall not be liable for
// any incidental or consequential damages arising out of the use or inability
// to use the software.

#include <iostream>
#include <unistd.h>

// headers from Connext DDS Micro/Cert installation
#include "rti_me_c.h"
#include "disc_dpse/disc_dpse_dpsediscovery.h"
#include "wh_sm/wh_sm_history.h"
#include "rh_sm/rh_sm_history.h"
#include "netio/netio_udp.h"

// rtiddsgen generated headers
#include "example.h"
#include "examplePlugin.h"
#include "exampleSupport.h"

#include "common_config.h"

extern "C" void my_typeSubscriber_on_data_available(
        void *listener_data,
        DDS_DataReader * reader)
{
    (void)(listener_data);  // to suppress -Wunused-parameter warning

    auto hw_reader = my_typeDataReader_narrow(reader);
    struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
    struct my_typeSeq sample_seq = DDS_SEQUENCE_INITIALIZER;
    const DDS_Long MAX_SAMPLES_PER_TAKE = 32;
    DDS_ReturnCode_t retcode;

    retcode = my_typeDataReader_take(
            hw_reader, 
            &sample_seq, 
            &info_seq, 
            MAX_SAMPLES_PER_TAKE, 
            DDS_ANY_SAMPLE_STATE, 
            DDS_ANY_VIEW_STATE, 
            DDS_ANY_INSTANCE_STATE);
    if (retcode != DDS_RETCODE_OK) {
        std::cout << "ERROR: failed to take data, retcode = " 
                << retcode << std::endl;
    }

    // print each valid sample taken
    DDS_Long i;
    for (i = 0; i < my_typeSeq_get_length(&sample_seq); ++i) {
        struct DDS_SampleInfo *sample_info = 
                DDS_SampleInfoSeq_get_reference(&info_seq, i);
        if (sample_info->valid_data) {
            my_type *sample = my_typeSeq_get_reference(&sample_seq, i);

            std::cout << "\nValid sample received" << std::endl;
            std::cout << "\tsample id = " << sample->id << std::endl;
            std::cout << "\tsample msg = " << sample->msg << std::endl;
        } else {
            std::cout << "\nSample received\n\tINVALID DATA" << std::endl;
        }
    }
    my_typeDataReader_return_loan(hw_reader, &sample_seq, &info_seq);
}

int main(void)
{
    DDS_ReturnCode_t retcode;

    // create the DomainParticipantFactory and registry so that we can make some 
    // changes to the default values
    auto dpf = DDS_DomainParticipantFactory_get_instance();
    auto registry = DDS_DomainParticipantFactory_get_registry(dpf);

    // register writer history
    if (!RT_Registry_register(
            registry, 
            DDSHST_WRITER_DEFAULT_HISTORY_NAME,
            WHSM_HistoryFactory_get_interface(), 
            NULL, 
            NULL))
    {
        std::cout << "ERROR: failed to register wh" << std::endl;
    }
    // register reader history
    if (!RT_Registry_register(
            registry, 
            DDSHST_READER_DEFAULT_HISTORY_NAME,
            RHSM_HistoryFactory_get_interface(), 
            NULL, 
            NULL))
    {
        std::cout << "ERROR: failed to register rh" << std::endl;
    }

    // Set up the UDP transport's allowed interfaces. To do this we:
    // (1) unregister the UDP transport
    // (2) name the allowed interfaces
    // (3) re-register the transport
    if (!RT_Registry_unregister(
            registry, 
            NETIO_DEFAULT_UDP_NAME, 
            NULL, 
            NULL)) 
    {
        std::cout << "ERROR: failed to unregister udp" << std::endl;
    }

    auto udp_property = (struct UDP_InterfaceFactoryProperty *)malloc(
            sizeof(struct UDP_InterfaceFactoryProperty));
    if (udp_property == NULL) {
        std::cout << "ERROR: failed to allocate udp properties" << std::endl;
    }
    *udp_property = UDP_INTERFACE_FACTORY_PROPERTY_DEFAULT;
    udp_property->disable_auto_interface_config = RTI_TRUE;
    
    if (!DDS_StringSeq_set_maximum(&udp_property->allow_interface,2)) {
        printf("failed to set allow_interface maximum\n");
        return -1;
    }
    if (!DDS_StringSeq_set_length(&udp_property->allow_interface,2)) {
        printf("failed to set allow_interface length\n");
        return -1;
    }

    if (!UDP_InterfaceTableEntrySeq_set_maximum(&udp_property->if_table,2)) {
        printf("failed to set if_table maximum\n");
        return -1;
    }

    *DDS_StringSeq_get_reference(&udp_property->allow_interface,0) = 
            DDS_String_dup(k_loopback_name.c_str());
    if (!UDP_InterfaceTable_add_entry(
            &udp_property->if_table,
            k_loopback_ip,
            k_loopback_mask,
            k_loopback_name.c_str(),
            UDP_INTERFACE_INTERFACE_UP_FLAG)) 
    {
        std::cout << "ERROR: failed to add " << k_loopback_name.c_str() <<
                "interface" << std::endl;
        return -1;
    }

    *DDS_StringSeq_get_reference(&udp_property->allow_interface,1) = 
            DDS_String_dup(k_real_nic_name.c_str());
    if (!UDP_InterfaceTable_add_entry(
            &udp_property->if_table,
            k_real_nic_ip,
            k_real_nic_mask,
            k_real_nic_name.c_str(),
            UDP_INTERFACE_INTERFACE_UP_FLAG)) 
    {
        std::cout << "ERROR: failed to add " << k_loopback_name.c_str() <<
                "interface" << std::endl;
        return -1;
    }

    //explicitly set the "real NIC" as the multicast interface
    udp_property->multicast_interface = DDS_String_dup(k_real_nic_name.c_str());

    if(!RT_Registry_register(
            registry, 
            NETIO_DEFAULT_UDP_NAME,
            UDP_InterfaceFactory_get_interface(),
            (struct RT_ComponentFactoryProperty*)udp_property, NULL))
    {
        std::cout << "ERROR: failed to re-register udp" << std::endl;
    } 

    // register the dpse (discovery) component
    struct DPSE_DiscoveryPluginProperty discovery_plugin_properties =
            DPSE_DiscoveryPluginProperty_INITIALIZER;
    if (!RT_Registry_register(
            registry,
            "dpse",
            DPSE_DiscoveryFactory_get_interface(),
            &discovery_plugin_properties._parent, 
            NULL))
    {
        std::cout << "ERROR: failed to register dpse" << std::endl;
    }

    // Now that we've finished the changes to the registry, we will start 
    // creating DDS entities. By setting autoenable_created_entities to false 
    // until all of the DDS entities are created, we limit all dynamic memory 
    // allocation to happen *before* the point where we enable everything.
    struct DDS_DomainParticipantFactoryQos dpf_qos =
            DDS_DomainParticipantFactoryQos_INITIALIZER;
    DDS_DomainParticipantFactory_get_qos(dpf, &dpf_qos);
    dpf_qos.entity_factory.autoenable_created_entities = DDS_BOOLEAN_FALSE;
    DDS_DomainParticipantFactory_set_qos(dpf, &dpf_qos);

    // configure discovery prior to creating our DomainParticipant
    struct DDS_DomainParticipantQos dp_qos = 
            DDS_DomainParticipantQos_INITIALIZER;
    if(!RT_ComponentFactoryId_set_name(
            &dp_qos.discovery.discovery.name,
            "dpse"))
    {
        std::cout << "ERROR: failed to set discovery plugin name" << std::endl;
    }
    if(!DDS_StringSeq_set_maximum(&dp_qos.discovery.initial_peers, 1)) {
        std::cout << "ERROR: failed to set initial peers maximum" << std::endl;
    }
    if (!DDS_StringSeq_set_length(&dp_qos.discovery.initial_peers, 1)) {
        std::cout << "ERROR: failed to set initial peers length" << std::endl;
    }
    *DDS_StringSeq_get_reference(&dp_qos.discovery.initial_peers, 0) = 
            DDS_String_dup(k_subscriber_initial_peer.c_str());

    // configure the DomainParticipant's resource limits... these are just 
    // examples, if there are more remote or local endpoints these values would
    // need to be increased
    dp_qos.resource_limits.max_destination_ports = 32;
    dp_qos.resource_limits.max_receive_ports = 32;
    dp_qos.resource_limits.local_topic_allocation = 1;
    dp_qos.resource_limits.local_type_allocation = 1;
    dp_qos.resource_limits.local_reader_allocation = 1;
    dp_qos.resource_limits.local_writer_allocation = 1;
    dp_qos.resource_limits.remote_participant_allocation = 8;
    dp_qos.resource_limits.remote_reader_allocation = 8;
    dp_qos.resource_limits.remote_writer_allocation = 8;

    //  set the name of the local DomainParticipant
    // (this is required for DPSE discovery)
    strcpy(dp_qos.participant_name.name, k_PARTICIPANT02_NAME.c_str());

    // now the DomainParticipant can be created
    auto dp = DDS_DomainParticipantFactory_create_participant(
            dpf, 
            domain_id,
            &dp_qos, 
            NULL,
            DDS_STATUS_MASK_NONE);
    if(dp == NULL) {
        std::cout << "ERROR: failed to create participant" << std::endl;
    }

    // register the type (my_type, from the idl) with the middleware
    std::string type_name = "my_type";
    retcode = DDS_DomainParticipant_register_type(
            dp,
            type_name.c_str(),
            my_typeTypePlugin_get());
    if(retcode != DDS_RETCODE_OK) {
        std::cout << "ERROR: failed to register type" << std::endl;
    }

    // Create the Topic to which we will publish. Note that the name of the 
    // Topic is stored in my-topic-name, which was defined in the IDL 
    auto topic = DDS_DomainParticipant_create_topic(
            dp,
            my_topic_name,
            type_name.c_str(),
            &DDS_TOPIC_QOS_DEFAULT, 
            NULL,
            DDS_STATUS_MASK_NONE);
    if(topic == NULL) {
        std::cout << "ERROR: topic == NULL" << std::endl;
    }

    // assert remote DomainParticipant
    retcode = DPSE_RemoteParticipant_assert(dp, k_PARTICIPANT01_NAME.c_str());
    if(retcode != DDS_RETCODE_OK) {
        std::cout << "ERROR: failed to assert remote participant" << std::endl;
    }

    // create the Subscriber
    auto subscriber = DDS_DomainParticipant_create_subscriber(
            dp,
            &DDS_SUBSCRIBER_QOS_DEFAULT,
            NULL, 
            DDS_STATUS_MASK_NONE);
    if(subscriber == NULL) {
        std::cout << "ERROR: subscriber == NULL" << std::endl;
    }

    // Create a listener to pass to the DataReader when we create it
    struct DDS_DataReaderListener dr_listener =
            DDS_DataReaderListener_INITIALIZER;
    dr_listener.on_data_available = my_typeSubscriber_on_data_available;

    // Configure the DataReader's QoS, then create the DataReader
    struct DDS_DataReaderQos dr_qos = DDS_DataReaderQos_INITIALIZER;

    dr_qos.protocol.rtps_object_id = k_OBJ_ID_PARTICIPANT02_DR01;
    dr_qos.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
    dr_qos.resource_limits.max_instances = 2;
    dr_qos.resource_limits.max_samples_per_instance = 32;
    dr_qos.resource_limits.max_samples = dr_qos.resource_limits.max_instances *
            dr_qos.resource_limits.max_samples_per_instance;
    dr_qos.reader_resource_limits.max_remote_writers = 10;
    dr_qos.reader_resource_limits.max_remote_writers_per_instance = 10;
    dr_qos.history.depth = 16;

    auto datareader = DDS_Subscriber_create_datareader(
            subscriber,
            DDS_Topic_as_topicdescription(topic), 
            &dr_qos,
            &dr_listener,
            DDS_DATA_AVAILABLE_STATUS);
    if(datareader == NULL) {
        std::cout << "ERROR: datareader == NULL" << std::endl;
    }

    // setup information about the publisher we are expecting to discover 
    struct DDS_PublicationBuiltinTopicData rem_publication_data =
        DDS_PublicationBuiltinTopicData_INITIALIZER;
    rem_publication_data.key.value[DDS_BUILTIN_TOPIC_KEY_OBJECT_ID] = 
            k_OBJ_ID_PARTICIPANT01_DW01;
    rem_publication_data.topic_name = DDS_String_dup(my_topic_name);
    rem_publication_data.type_name = DDS_String_dup(type_name.c_str());
    rem_publication_data.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;    

    // now assert the publisher so the local DP knows that we expect to find it
    retcode = DPSE_RemotePublication_assert(
            dp,
            k_PARTICIPANT01_NAME.c_str(),
            &rem_publication_data,
            my_type_get_key_kind(my_typeTypePlugin_get(), 
            NULL));
    if (retcode != DDS_RETCODE_OK) {
        std::cout << "ERROR: failed to assert remote publication" << std::endl;
    }

    // Finally, now that all of the entities are created, we can enable them all
    retcode = DDS_Entity_enable(DDS_DomainParticipant_as_entity(dp));
    if(retcode != DDS_RETCODE_OK) {
        std::cout << "ERROR: failed to enable entity" << std::endl;
    }

    std::cout << "Waiting for samples to arrive, press Ctrl-C to exit" 
            << std::endl;
    while(1) {
        // optional work could be done here
        sleep(10); // sleep for 10s, then loop again
    }    
}

