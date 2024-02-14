/*
*   Copyright 2022-2023 Visa
*
*   This code is licensed under the Creative Commons
*   Attribution-NonCommercial 4.0 International Public License 
*   (https://creativecommons.org/licenses/by-nc/4.0/legalcode).
*/
#include "drop-ot/UnitTests.h"
#include "cryptoTools/Network/IOService.h"
#include "cryptoTools/Network/Session.h"
#include "cryptoTools/Common/Timer.h"
#include "cryptoTools/Common/CLP.h"
#include "cryptoTools/Common/Log.h"
#include "cryptoTools/Common/Matrix.h"
#include <cassert>
#include "drop-ot/IknpOtExt.h"
#include "drop-ot/MasnyRindal.h"
#include <fstream>

using namespace dropOt;

// void tutorial(oc::CLP& cmd);
void networked(oc::CLP& cmd);
// void help(oc::CLP& cmd);



struct Ot{



// The first function to be called. This generates a message to be sent to the
// other party. Intermediate state is written to stateDir directory.
std::vector<u8> sender_Setup1(std::string stateDir, std::string bankID)
{
    MasnyRindal mr;
    PRNG prng(oc::sysRandomSeed());

    std::vector<u8> ioBuffer;

    // we use random choices here.
    oc::BitVector choices(128);
    choices.randomize(prng);

    // run the first part of the protocol.
    mr.receiveRoundOne(choices, prng, ioBuffer);

    // serialize the state.
    std::ofstream out;
    out.open(stateDir + "/senderState" + bankID +".bin", std::ios::trunc | std::ios::binary | std::ios::out);

    // char* var = "adsad";
    // out.open(var + "/senderState.bin", std::ios::trunc | std::ios::binary | std::ios::out);
    mr.serialize(out);


    // return the message to be sent.
    return ioBuffer;
}


// The first function to be called by the sender. It take as input the stateDir 
// and the message generated by recver_Setup(...). After this the sender is
// setup and ready to generate OT keys. Intermediate state is written to stateDir directory.
void sender_Setup2(
    std::string stateDir,
    std::string bankID,
    span<u8> ioBuffer)
{
    MasnyRindal mr;
    PRNG prng(oc::sysRandomSeed());

    std::ifstream in;
    in.open(stateDir + "/senderState" + bankID +".bin", std::ios::binary | std::ios::in);
    mr.deserialize(in);
    in.close();

    auto choices = mr.mChoices;
    std::remove((stateDir + "/senderState" + bankID +".bin").c_str());

    std::vector<block> keys(choices.size());
    mr.receiveRoundTwo(keys, prng, ioBuffer);

    IknpOtExtSender sender;
    sender.setUniformBaseOts(keys, choices);
    std::ofstream out;
    out.open(stateDir + "/senderState" + bankID +".bin", std::ios::trunc | std::ios::binary | std::ios::out);
    sender.serialize(out);
}



// The setup function for the receiver. It take as input the stateDir 
// and the message generated by sender_Setup1(...). After this the receiver is
// setup and ready to generate OT keys. Intermediate state is written to stateDir directory.
// It returns a message to be sent to the sender to complete their setup.
std::vector<u8> recver_Setup(
    std::string stateDir,
    std::string bankID,
    span<u8> inMessage)
{
    MasnyRindal mr;
    PRNG prng(oc::sysRandomSeed());

    std::vector<std::array<block, 2>> keys(128);

    std::vector<u8> ioBuffer;
    mr.sendRoundOne(128, prng, ioBuffer);
    mr.sendRoundTwo(keys, prng, inMessage);

    IknpOtExtReceiver recver;
    recver.setUniformBaseOts(keys);

    
    std::ofstream out;
    out.open(stateDir + "/recverState" + bankID +".bin", std::ios::trunc | std::ios::binary | std::ios::out);
    recver.serialize(out);

    return ioBuffer;
}


// Perform the main protocol for the OT receiver. This generates useable OT keys. It takes as input stateDir,
// where the receiver state is serialized, and it takes as input choices which are the
// user provided choices values for the OTs. It writes the output OT keys to recverKeys.
// It returns a message to be sent to the sender.
std::vector<u8> recver_generateKeys(std::string stateDir, std::string bankID, std::vector<block>& recverKeys, BitVector& choices)
{
    if (recverKeys.size() != choices.size())
        throw RTE_LOC;

    std::ifstream in;
    in.open(stateDir + "/recverState" + bankID +".bin", std::ios::binary | std::ios::in);

    IknpOtExtReceiver recver;
    recver.deserialize(in);



    PRNG prng(oc::sysRandomSeed());
    std::vector<u8> ioBuffer;
    recver.receiveRoundOne_s(choices, recverKeys, prng, ioBuffer);

    std::ofstream out;
    out.open(stateDir + "/recverState" + bankID +".bin", std::ios::trunc | std::ios::binary | std::ios::out);
    recver.serialize(out);

    return ioBuffer;
}



// Perform the main protocol for the OT sender. This generates useable OT keys. It takes as input stateDir,
// where the sender state is serialized. It writes the output OT keys to senderKeys.
void sender_generateKeys(std::string stateDir, std::string bankID, span<u8>inMessages, std::vector<std::array<block, 2>>& senderKeys)
{
    std::ifstream in;
    in.open(stateDir + "/senderState" + bankID +".bin", std::ios::binary | std::ios::in);

    IknpOtExtSender sender;
    sender.deserialize(in);

    PRNG prng(oc::sysRandomSeed());
    sender.sendRoundOne_r(senderKeys, prng, inMessages);

    std::ofstream out;
    out.open(stateDir + "/senderState" + bankID +".bin", std::ios::trunc | std::ios::binary | std::ios::out);
    sender.serialize(out);
}



};


// int main(int argc, char** argv)
// {
//     oc::CLP cmd(argc, argv);

//     if (cmd.isSet("u"))
//         unitTests.runIf(cmd);
//     else if (cmd.isSet("tut"))
//         tutorial(cmd);
//     else
//         help(cmd);

//     return 0;
// }

// void help(oc::CLP& cmd)
// {
//     std::cout << "-u to run unit tests" << std::endl;
//     std::cout << "-tut to run tutorial" << std::endl;

// }

// // The overall flow is shown here.
// void tutorial(oc::CLP& cmd)
// {
//     // Ot ot;

//     // auto stateDir = cmd.getOr<std::string>("stateDir", ".");
//     // auto n = cmd.getOr<u64>("n", 100);
//     // auto trials = cmd.getOr<u64>("t", 2);


//     // //////////////////////////////
//     // // setup

//     // auto buffer = ot.sender_Setup1(stateDir);

//     // buffer = ot.recver_Setup(stateDir, span<u8>(buffer));

//     // ot.sender_Setup2(stateDir, span<u8>(buffer));


//     // //////////////////////////////
//     // // main

//     // for (u64 t = 0; t < trials; ++t)
//     // {

//     //     // these will be input in the real code...
//     //     oc::BitVector choices(n);
//     //     for (u64 i = 0; i < choices.size(); ++i)
//     //         choices[i] = i % 2;

//     //     // output keys
//     //     std::vector<block> recverKeys(n);

//     //     buffer = ot.recver_generateKeys(stateDir, recverKeys, choices);

//     //     std::vector<std::array<block, 2>> senderKeys(n);
//     //     ot.sender_generateKeys(stateDir, span<u8>(buffer), senderKeys);


//     //     std::cout << "generated OT keys" << std::endl;
//     //     for (u64 i = 0; i < n; ++i)
//     //     {
//     //         std::cout << "sender {" << senderKeys[i][0] << ", " << senderKeys[i][1] << "}, recver{" << recverKeys[i] << ", " << choices[i] << "}" << std::endl;
//     //     }
//     // }
// }