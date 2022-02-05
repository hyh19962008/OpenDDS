#include <dds/DCPS/InternalDataReader.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace OpenDDS::DCPS;

struct Sample {
  std::string key;

  Sample(const std::string& a_key)
    : key(a_key)
  {}

  bool operator==(const Sample& other) const
  {
    return key == other.key;
  }

  bool operator<(const Sample& other) const
  {
    return key < other.key;
  }
};

typedef InternalDataReader<Sample> ReaderType;

class Listener : public InternalDataReaderListener<Sample> {
public:
  Listener(JobQueue_rch job_queue)
    : InternalDataReaderListener<Sample>(job_queue)
  {}

  MOCK_METHOD1(on_data_available, void(RcHandle<ReaderType>));
};

TEST(dds_DCPS_InternalDataReader, durable)
{
  {
    RcHandle<ReaderType> reader = make_rch<ReaderType>(false);
    EXPECT_FALSE(reader->durable());
  }
  {
    RcHandle<ReaderType> reader = make_rch<ReaderType>(true);
    EXPECT_TRUE(reader->durable());
  }
}

TEST(dds_DCPS_InternalDataReader, register_instance)
{
  int publication;
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<ReaderType> reader = make_rch<ReaderType>(false);

  reader->register_instance(&publication, sample);
  reader->take(samples, infos);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(infos[0], InternalSampleInfo(ISIK_REGISTER, &publication));
}

TEST(dds_DCPS_InternalDataReader, write)
{
  int publication;
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<ReaderType> reader = make_rch<ReaderType>(false);

  reader->write(&publication, sample);
  reader->take(samples, infos);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(infos[0], InternalSampleInfo(ISIK_SAMPLE, &publication));
}

TEST(dds_DCPS_InternalDataReader, unregister_instance)
{
  int publication;
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<ReaderType> reader = make_rch<ReaderType>(false);

  reader->register_instance(&publication, sample);
  reader->unregister_instance(&publication, sample);
  reader->take(samples, infos);

  ASSERT_EQ(samples.size(), 2U);
  ASSERT_EQ(infos.size(), 2U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(infos[0], InternalSampleInfo(ISIK_REGISTER, &publication));

  EXPECT_EQ(samples[1], sample);
  EXPECT_EQ(infos[1], InternalSampleInfo(ISIK_UNREGISTER, &publication));
}

TEST(dds_DCPS_InternalDataReader, dispose)
{
  int publication;
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<ReaderType> reader = make_rch<ReaderType>(false);

  reader->register_instance(&publication, sample);
  reader->dispose(&publication, sample);
  reader->take(samples, infos);

  ASSERT_EQ(samples.size(), 2U);
  ASSERT_EQ(infos.size(), 2U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(infos[0], InternalSampleInfo(ISIK_REGISTER, &publication));

  EXPECT_EQ(samples[1], sample);
  EXPECT_EQ(infos[1], InternalSampleInfo(ISIK_DISPOSE, &publication));
}

TEST(dds_DCPS_InternalDataReader, remove_publication)
{
  int publication;
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<ReaderType> reader = make_rch<ReaderType>(false);

  reader->register_instance(&publication, sample);
  reader->remove_publication(&publication);
  reader->take(samples, infos);

  ASSERT_EQ(samples.size(), 2U);
  ASSERT_EQ(infos.size(), 2U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(infos[0], InternalSampleInfo(ISIK_REGISTER, &publication));

  EXPECT_EQ(samples[1], sample);
  EXPECT_EQ(infos[1], InternalSampleInfo(ISIK_UNREGISTER, &publication));
}

TEST(dds_DCPS_InternalDataReader, listener)
{
  int publication;
  Sample sample("key");

  JobQueue_rch job_queue = make_rch<JobQueue>(ACE_Reactor::instance());
  RcHandle<Listener> listener = make_rch<Listener>(job_queue);
  RcHandle<ReaderType> reader = make_rch<ReaderType>(false,
                                                     listener);

  EXPECT_EQ(reader->get_listener(), listener);

  // This increases the reference count on reader which prevents the destruction and check of the listener.
  EXPECT_CALL(*listener.get(), on_data_available(reader));

  reader->register_instance(&publication, sample);

  ACE_Time_Value tv(1,0);
  ACE_Reactor::run_event_loop(tv);

  reader->set_listener(RcHandle<Listener>());
}
