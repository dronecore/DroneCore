#include "dronecore.h"
#include "curl_wrapper.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <gtest/gtest.h>

using namespace dronecore;

class CurlTest : public testing::Test
{
protected:
    std::string _file_url_existing_http;
    std::string _file_url_existing_https;
    std::string _file_url_not_existing;
    std::string _local_path;

    virtual void SetUp()
    {
        clean();
        _file_url_existing_http =
            "http://s3.eu-central-1.amazonaws.com/404f358a-48b5-4aaf-b5fd-adc84ffb0f31/dronecore_test_file";
        _file_url_existing_https =
            "https://s3.eu-central-1.amazonaws.com/404f358a-48b5-4aaf-b5fd-adc84ffb0f31/dronecore_test_file";
        _file_url_not_existing = "https://notexisting.file/does-really-not-exist";
        _local_path = "testfile.txt";
    }

    virtual void TearDown()
    {
        clean();
    }

    void clean()
    {
        remove(_local_path.c_str());
    }

    bool check_file_exists(const std::string &file_path)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::ifstream infile(file_path.c_str());
        return infile.good();
    }
};

TEST_F(CurlTest, Curl_DownloadText_HTTP_Success)
{
    std::string content;

    CurlWrapper curl_wrapper;
    bool success = curl_wrapper.download_text(_file_url_existing_http, content);

    EXPECT_EQ(success, true);
    EXPECT_EQ(content, "content of this file\n");
}

TEST_F(CurlTest, Curl_DownloadText_HTTPS_Success)
{
    std::string content;

    CurlWrapper curl_wrapper;
    bool success = curl_wrapper.download_text(_file_url_existing_https, content);

    EXPECT_EQ(success, true);
    EXPECT_EQ(content, "content of this file\n");
}

TEST_F(CurlTest, Curl_DownloadText_FileNotFound)
{
    std::string content = "content gets cleared if download fails";

    CurlWrapper curl_wrapper;
    bool success = curl_wrapper.download_text(_file_url_not_existing, content);

    EXPECT_EQ(success, false);
    EXPECT_EQ(content, "");
}

TEST_F(CurlTest, Curl_DownloadFile_WithoutProgressFeedback_Success)
{
    CurlWrapper curl_wrapper;

    bool success = curl_wrapper.download_file_to_path(_file_url_existing_https, _local_path, nullptr);
    EXPECT_EQ(success, true);

    bool file_exists = check_file_exists(_local_path);
    EXPECT_EQ(file_exists, true);
}

TEST_F(CurlTest, Curl_DownloadFile_WithoutProgressFeedback_FileNotFound)
{
    CurlWrapper curl_wrapper;

    bool success = curl_wrapper.download_file_to_path(_file_url_not_existing, _local_path, nullptr);
    EXPECT_EQ(success, false);

    bool file_exists = check_file_exists(_local_path);
    EXPECT_EQ(file_exists, false);
}

TEST_F(CurlTest, Curl_DownloadFile_ProgressFeedback_Success)
{
    int last_progress;
    Status last_status;
    CURLcode last_curl_code;

    auto progress = [&last_progress, &last_status, &last_curl_code]
    (int got_progress, Status status, CURLcode curl_code) -> int {
        last_progress = got_progress;
        last_status = status;
        last_curl_code = curl_code;
        return 0;
    };

    CurlWrapper curl_wrapper;

    bool success = curl_wrapper.download_file_to_path(_file_url_existing_https, _local_path, progress);
    EXPECT_EQ(success, true);
    EXPECT_EQ(last_progress, 100);
    EXPECT_EQ(last_status, Status::Finished);
    EXPECT_EQ(last_curl_code, CURLcode::CURLE_OK);

    bool file_exists = check_file_exists(_local_path);
    EXPECT_EQ(file_exists, true);
}

TEST_F(CurlTest, Curl_DownloadFile_ProgressFeedback_COULDNT_RESOLVE_HOST)
{
    int last_progress;
    Status last_status;
    CURLcode last_curl_code;

    auto progress = [&last_progress, &last_status, &last_curl_code]
    (int got_progress, Status status, CURLcode curl_code) -> int {
        last_progress = got_progress;
        last_status = status;
        last_curl_code = curl_code;
        return 0;
    };

    CurlWrapper curl_wrapper;

    bool success = curl_wrapper.download_file_to_path(_file_url_not_existing, _local_path, progress);
    EXPECT_EQ(success, false);
    EXPECT_EQ(last_progress, 0);
    EXPECT_EQ(last_status, Status::Error);
    EXPECT_EQ(last_curl_code, CURLcode::CURLE_COULDNT_RESOLVE_HOST);

    bool file_exists = check_file_exists(_local_path);
    EXPECT_EQ(file_exists, false);
}
