/* 
 * File:   TestUtility.cpp
 * Author: daniel
 * 
 * Created on December 9, 2015, 4:28 PM
 */

#include "TestUtility.h"

TestUtility::TestUtility()
{
    TEST_ADD(TestUtility::printDomainInfo);
    TEST_ADD(TestUtility::testNetworkInfo);
    TEST_ADD(TestUtility::testAddressFromHostName);
    TEST_ADD(TestUtility::testTrim);
    TEST_ADD(TestUtility::testEqualsIgnoreCase);
    TEST_ADD(TestUtility::testReplaceAll);
    TEST_ADD(TestUtility::testJoinStrings);
    TEST_ADD(TestUtility::testDecodeURI);
    TEST_ADD(TestUtility::testToHexString);
    TEST_ADD(TestUtility::testWaitForUserInput);
}

void TestUtility::printDomainInfo()
{
    std::cout << "Domain name: " << Utility::getDomainName() << std::endl
            << "User name: " << Utility::getUserName() << std::endl
            << "Loopback address: " << Utility::getLocalIPAddress(Utility::AddressType::ADDRESS_LOOPBACK) << std::endl
            << "Local address: " << Utility::getLocalIPAddress(Utility::AddressType::ADDRESS_LOCAL_NETWORK) << std::endl
            << "External address: " << Utility::getLocalIPAddress(Utility::AddressType::ADDRESS_INTERNET) << std::endl;
}

void TestUtility::testNetworkInfo()
{
    TEST_ASSERT_EQUALS(Utility::AddressType::ADDRESS_LOOPBACK, Utility::getNetworkType("127.0.0.1"));
    TEST_ASSERT_EQUALS(Utility::AddressType::ADDRESS_LOOPBACK, Utility::getNetworkType("::1"));
    TEST_ASSERT_EQUALS(Utility::AddressType::ADDRESS_LOCAL_NETWORK, Utility::getNetworkType("10.11.12.13"));
    TEST_ASSERT_EQUALS(Utility::AddressType::ADDRESS_LOCAL_NETWORK, Utility::getNetworkType("172.24.25.26"));
    TEST_ASSERT_EQUALS(Utility::AddressType::ADDRESS_LOCAL_NETWORK, Utility::getNetworkType("192.168.2.13"));
    TEST_ASSERT_EQUALS(Utility::AddressType::ADDRESS_LOCAL_NETWORK, Utility::getNetworkType("fd00::d250:99ff:fe5e:4907"));
    TEST_ASSERT_EQUALS(Utility::AddressType::ADDRESS_INTERNET, Utility::getNetworkType("81.81.81.81"));
    TEST_ASSERT_EQUALS(Utility::AddressType::ADDRESS_INTERNET, Utility::getNetworkType("7.7.7.7"));
    TEST_ASSERT_EQUALS(Utility::AddressType::ADDRESS_INTERNET, Utility::getNetworkType("2002:ADC2:712F:0:0:0:0:0"));
    TEST_ASSERT_EQUALS(Utility::AddressType::ADDRESS_INTERNET, Utility::getNetworkType("::ADC2:712F"));
}

void TestUtility::testAddressFromHostName()
{
    TEST_ASSERT(Utility::getAddressForHostName("checkip.dyndns.org").compare("91.198.22.70") == 0);
}

void TestUtility::testTrim()
{
    TEST_ASSERT_EQUALS(std::string("trimmed text"), Utility::trim("  trimmed text   "));
}

void TestUtility::testEqualsIgnoreCase()
{
    TEST_ASSERT(Utility::equalsIgnoreCase(std::string("some Text"), std::string("SoMe tExT")));
}

void TestUtility::testReplaceAll()
{
    TEST_ASSERT(Utility::replaceAll("abababc", "b", "a").compare("aaaaaac") == 0);
    TEST_ASSERT(Utility::replaceAll("abababc", "d", "a").compare("abababc") == 0);
    TEST_ASSERT(Utility::replaceAll("abababc", "ba", "a").compare("aaabc") == 0);
    TEST_ASSERT(Utility::replaceAll("abababc", "b", "ba").compare("abaabaabac") == 0);
}

void TestUtility::testJoinStrings()
{
    TEST_ASSERT(Utility::joinStrings({"one","two"}, " and ").compare("one and two") == 0);
    TEST_ASSERT(Utility::joinStrings({"beer", "fest", "ival"}, "").compare("beerfestival") == 0);
}

void TestUtility::testDecodeURI()
{
    TEST_ASSERT(Utility::decodeURI("https%3A%2F%2Fgithub.com%2Fdoe300%2FOHMComm%2Fissues%2F64").compare("https://github.com/doe300/OHMComm/issues/64") == 0);
    TEST_ASSERT(Utility::decodeURI("https://github.com/doe300/OHMComm/issues/64").compare("https://github.com/doe300/OHMComm/issues/64") == 0);
}

void TestUtility::testToHexString()
{
    TEST_ASSERT(Utility::toHexString(1234567890).compare("499602D2") == 0);
}

void TestUtility::testWaitForUserInput()
{
    TEST_ASSERT_EQUALS(-1, Utility::waitForUserInput(500));
    TEST_ASSERT_EQUALS(-1, Utility::waitForUserInput(5));
}
