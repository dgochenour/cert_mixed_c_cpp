// This sample code is intended to show that the Connext DDS Micro 2.4.12
// C API can be called from a C++ application. It is STRICTLY an example and
// NOT intended to represent production-quality code.

#include <iostream>
#include <sstream>

// headers from Connext DDS Micro installation
#include "rti_me_c.h"
#include "disc_dpse/disc_dpse_dpsediscovery.h"
#include "wh_sm/wh_sm_history.h"
#include "rh_sm/rh_sm_history.h"
#include "netio/netio_udp.h"

// rtiddsgen generated headers
#include "example.h"
#include "examplePlugin.h"
#include "exampleSupport.h"


int main(void)
{
    // user-configurable values
    std::string peer = "127.0.0.1";
    std::string loopback_name = "lo";
    std::string eth_nic_name = "wlp0s20f3";
    std::string local_participant_name = "publisher";
    std::string remote_participant_name = "subscriber";
    auto domain_id = 100;

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
    // (1) unregister the UDP trasport
    // (2) name the allowed interfaces
    // (3) re-register the transport
    if(!RT_Registry_unregister(
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

    // For additional allowed interface(s), increase maximum and length, and
    // set interface below:
    REDA_StringSeq_set_maximum(&udp_property->allow_interface,2);
    REDA_StringSeq_set_length(&udp_property->allow_interface,2);
    *REDA_StringSeq_get_reference(&udp_property->allow_interface,0) = 
            DDS_String_dup(loopback_name.c_str()); 
    *REDA_StringSeq_get_reference(&udp_property->allow_interface,1) = 
            DDS_String_dup(eth_nic_name.c_str()); 

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

    // Now that we've finsihed the changes to the registry, we will start 
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
            DDS_String_dup(peer.c_str());

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
    strcpy(dp_qos.participant_name.name,local_participant_name.c_str());

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
    retcode = DPSE_RemoteParticipant_assert(
            dp, 
            remote_participant_name.c_str());
    if(retcode != DDS_RETCODE_OK) {
        std::cout << "ERROR: failed to assert remote participant" << std::endl;
    }

    // create the Publisher
    auto publisher = DDS_DomainParticipant_create_publisher(
            dp,
            &DDS_PUBLISHER_QOS_DEFAULT,
            NULL,
            DDS_STATUS_MASK_NONE);
    if(publisher == NULL) {
        std::cout << "ERROR: Publisher == NULL" << std::endl;
    }

    // Configure the DataWriter's QoS, then create the DataWriter
    struct DDS_DataWriterQos dw_qos = DDS_DataWriterQos_INITIALIZER;

    dw_qos.protocol.rtps_object_id = 100;
    dw_qos.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
    dw_qos.resource_limits.max_samples_per_instance = 32;
    dw_qos.resource_limits.max_instances = 2;
    dw_qos.resource_limits.max_samples = dw_qos.resource_limits.max_instances *
            dw_qos.resource_limits.max_samples_per_instance;
    dw_qos.history.depth = 16;
    dw_qos.protocol.rtps_reliable_writer.heartbeat_period.sec = 0;
    dw_qos.protocol.rtps_reliable_writer.heartbeat_period.nanosec = 250000000;

    auto datawriter = DDS_Publisher_create_datawriter(
            publisher, 
            topic, 
            &dw_qos,
            NULL,
            DDS_STATUS_MASK_NONE);
    if(datawriter == NULL) {
        std::cout << "ERROR: datawriter == NULL" << std::endl;
    }   

    // setup information about the subscriber we are expecting to discover 
    struct DDS_SubscriptionBuiltinTopicData rem_subscription_data =
            DDS_SubscriptionBuiltinTopicData_INITIALIZER;
    rem_subscription_data.key.value[DDS_BUILTIN_TOPIC_KEY_OBJECT_ID] = 200;
    rem_subscription_data.topic_name = DDS_String_dup(my_topic_name);
    rem_subscription_data.type_name = DDS_String_dup(type_name.c_str());
    rem_subscription_data.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;

    retcode = DPSE_RemoteSubscription_assert(
            dp,
            remote_participant_name.c_str(),
            &rem_subscription_data,
            my_type_get_key_kind(my_typeTypePlugin_get(), NULL));
    if (retcode != DDS_RETCODE_OK) {
        std::cout << "ERROR: failed to assert remote publication" << std::endl;
    }    

    // create the data sample that we will write
    auto sample = my_type_create();
    if(sample == NULL) {
        std::cout << "ERROR: failed my_type_create" << std::endl;
    }

    // Finaly, now that all of the entities are created, we can enable them all
    auto entity = DDS_DomainParticipant_as_entity(dp);
    retcode = DDS_Entity_enable(entity);
    if(retcode != DDS_RETCODE_OK) {
        std::cout << "ERROR: failed to enable entity" << std::endl;
    }

    // Now we can narrow (downcast) the DataWriter and write some samples
    auto hw_datawriter = my_typeDataWriter_narrow(datawriter);
    auto i = 0;
    while (1) {
        
        // add some data to the sample
        std::ostringstream msg;  
        msg << "sample #" << i;
        msg.str().copy(sample->msg, 128);

        retcode = my_typeDataWriter_write(
                hw_datawriter, 
                sample, 
                &DDS_HANDLE_NIL);
        if(retcode != DDS_RETCODE_OK) {
            std::cout << "ERROR: Failed to write sample" << std::endl;
        } else {
            std::cout << "Wrote sample " << i << std::endl;
            i++;
        } 
        OSAPI_Thread_sleep(1000); // sleep 1s between writes 
    }
}

