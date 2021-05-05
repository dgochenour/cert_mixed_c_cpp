// This sample code is intended to show that the Connext DDS Micro 2.4.12
// C API can be called from a C++ application. It is STRICTLY an example and
// NOT intended to represent production-quality code.

#include <iostream>

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

extern "C" void my_typeSubscriber_on_data_available(
        void *listener_data,
        DDS_DataReader * reader)
{
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
    my_typeSeq_finalize(&sample_seq);
    DDS_SampleInfoSeq_finalize(&info_seq);
}

int main(void)
{
    // user-configurable values
    std::string peer = "127.0.0.1";
    std::string loopback_name = "lo";
    std::string eth_nic_name = "wlp0s20f3";
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
    strcpy(dp_qos.participant_name.name, "subscriber");

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
            "publisher");
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

    dr_qos.protocol.rtps_object_id = 200;
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
    rem_publication_data.key.value[DDS_BUILTIN_TOPIC_KEY_OBJECT_ID] = 100;
    rem_publication_data.topic_name = DDS_String_dup(my_topic_name);
    rem_publication_data.type_name = DDS_String_dup(type_name.c_str());
    rem_publication_data.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;    

    // now assert the publisher so the local DP knows that we expect to find it
    retcode = DPSE_RemotePublication_assert(
            dp,
            "publisher",
            &rem_publication_data,
            my_type_get_key_kind(my_typeTypePlugin_get(), 
            NULL));
    if (retcode != DDS_RETCODE_OK) {
        std::cout << "ERROR: failed to assert remote publication" << std::endl;
    }

    // Finaly, now that all of the entities are created, we can enable them all
    auto entity = DDS_DomainParticipant_as_entity(dp);
    retcode = DDS_Entity_enable(entity);
    if(retcode != DDS_RETCODE_OK) {
        std::cout << "ERROR: failed to enable entity" << std::endl;
    }

    std::cout << "Waiting for samples to arrive, press Ctrl-C to exit" 
            << std::endl;
    while(1) {
        OSAPI_Thread_sleep(10000); // sleep for 10s, then loop again
    }    
}

