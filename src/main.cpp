#include <bits/stdc++.h>

#include <crypt.h>
#include <mpi.h>

#include "MPIJobData.hpp"

#define MASTER_RANK 0

using namespace std::literals;

/*
@brief Increments input string by 1 alphabet Only lower-case alphabetical string
@param old Input String
@return true if addition overflowed else false
*/
bool IncrementString(std::string &old)
{
    bool carry = true;
    auto it = old.rbegin();
    while (carry && it != old.rend())
    {
        *it += 1;
        if (*it > 'z')
            carry = true, *it = 'a';
        else
            carry = false;
        ++it;
    }
    return carry ? false : true;
}
/*
@brief Generates Uniformly distributed string
@param size No of partitions
@return Partitions
*/
std::vector<char[9]> GenerateUniformly(std::size_t size)
{
    if (size > 26)
        throw std::runtime_error("Size is greater than 26");
    auto partitionSize = 26 / size;
    auto partitions = std::vector<char[9]>(size);
    auto it = partitions.begin();
    for (auto i = 0ul; i < size * partitionSize; i += partitionSize)
    {
        auto ch = static_cast<char>(i + 'a');
        char partition[] = {ch, ch, ch, ch, ch, ch, ch, ch, 0};
        memcpy(*it, partition, 9);
        ++it;
    }
    return partitions;
}

/*
@brief Reads /etc/shadow file, gets hash and Initializes Job Data for each MPI Job
@param user The user to crack password for
@param numProcs No of MPI Jobs
@return All MPI Job Data
*/
std::vector<MPIJobData> Initialization(std::string user, uint numProcs)
{
    //Read File /etc/shadow containing passwords hash for all users
    auto passwdFile = std::ifstream("/etc/shadow");
    if (!passwdFile.is_open())
        throw std::runtime_error("Cannot Open Password File /etc/shadow");

    ///Find the entry for user
    std::string userData;
    while (std::getline(passwdFile, userData))
        if (userData.find(user) != std::string::npos)
            break;

    if (passwdFile.eof())
        throw std::runtime_error("Could'nt find user: " + user);

    //Get Password Hash for the user
    auto hash_start = userData.find(':') + 1;
    auto hash = userData.substr(hash_start, userData.find(':', hash_start) - hash_start);

    //Get salt used for generating hash
    auto setting = hash.substr(0, hash.find_last_of('$'));

    //Create equally distributed passwords for brute-forcing
    auto partitions = GenerateUniformly(numProcs);

    auto data = std::vector<MPIJobData>(numProcs);

    //Create Data for each MPI Job
    for (auto i = 0u; i < numProcs; ++i)
    {
        std::copy(setting.begin(), setting.end(), data[i].setting);
        std::copy(hash.begin(), hash.end(), data[i].originalHash);
        std::copy(partitions[i], partitions[i] + 9, data[i].startingPasswd);
        if (i != numProcs - 1)
            std::copy(partitions[i + 1], partitions[i + 1] + 9, data[i].endingPasswd);
        else
        {
            auto endingPasswd = "zzzzzzzz"sv;
            std::copy(endingPasswd.begin(), endingPasswd.end(), data[i].endingPasswd);
        }
    }
    return data;
}

int main(int argc, char **argv)
{
    std::stringstream out;
    //Print bool as (true/false)
    out << std::boolalpha;

    MPI::Init(argc, argv);
    if (MPI::Is_initialized())
    {
        auto size = MPI::COMM_WORLD.Get_size();
        auto rank = MPI::COMM_WORLD.Get_rank();

        auto data = std::vector<MPIJobData>();
        auto type = MPIJobData::MPIDataType();

        //Only MASTER_RANK initializes data
        //and scatters it to all ranks
        if (rank == MASTER_RANK)
            data = Initialization("mpiuser", size);

        //Local data for each MPI Job
        MPIJobData localData;
        //Scatter Data
        MPI::COMM_WORLD.Scatter(static_cast<void *>(data.data()), 1, type,
                                static_cast<void *>(&localData), 1, type,
                                MASTER_RANK);

        auto currentPasswd = std::string(localData.startingPasswd);
        auto endingPasswd = std::string(localData.endingPasswd);
        //Brute force each password until found
        while (currentPasswd != endingPasswd)
        {
            //Find Hash
            auto hash = std::string_view(crypt(currentPasswd.data(), localData.setting));
            if (hash == localData.originalHash)
            {
                ///TODO: Stops all other Jobs when 1 job finds password
                out << "Found Password: " << currentPasswd << '\n';
                break;
            }
            //Increment String by 1 alphabet (Only lower-case 8 letter string)
            IncrementString(currentPasswd);
        }

        MPI::Finalize();
    }
    //Flush any local MPI Job data to stdout
    std::cout << out.str();
}