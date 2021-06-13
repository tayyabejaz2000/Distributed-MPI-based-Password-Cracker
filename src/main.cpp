#include <bits/stdc++.h>
#include <crypt.h>
#include <mpi.h>

#include "MPIJobData.hpp"

#define MASTER_RANK 0

using namespace std::literals;

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
        char partition[9] = {ch, ch, ch, ch, ch, ch, ch, ch, 0};
        memcpy(*it, partition, 9);
        ++it;
    }
    return partitions;
}

std::vector<MPIJobData> Initialization(std::string user, uint numProcs)
{
    auto passwdFile = std::ifstream("/etc/shadow");
    if (!passwdFile.is_open())
        throw std::runtime_error("Cannot Open Password File /etc/shadow");

    std::string userData;
    while (std::getline(passwdFile, userData))
        if (userData.find(user) != std::string::npos)
            break;

    if (passwdFile.eof())
        throw std::runtime_error("Could'nt find user: " + user);

    auto hash_start = userData.find(':') + 1;
    auto hash = userData.substr(hash_start, userData.find(':', hash_start) - hash_start);

    auto setting = hash.substr(0, hash.find_last_of('$'));

    auto partitions = GenerateUniformly(numProcs);

    auto data = std::vector<MPIJobData>(numProcs);

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
    out << std::boolalpha;

    MPI::Init(argc, argv);
    if (MPI::Is_initialized())
    {
        auto size = MPI::COMM_WORLD.Get_size();
        auto rank = MPI::COMM_WORLD.Get_rank();

        auto data = std::vector<MPIJobData>();
        auto type = MPIJobData::MPIDataType();

        if (rank == MASTER_RANK)
            data = Initialization("mpiuser", size);

        MPIJobData localData;
        MPI::COMM_WORLD.Scatter(static_cast<void *>(data.data()), 1, type,
                                static_cast<void *>(&localData), 1, type,
                                MASTER_RANK);

        auto currentPasswd = std::string(localData.startingPasswd);
        auto endingPasswd = std::string(localData.endingPasswd);
        while (currentPasswd != endingPasswd)
        {
            auto hash = std::string_view(crypt(currentPasswd.data(), localData.setting));
            if (hash == localData.originalHash)
            {
                out << "Found Password: " << currentPasswd << '\n';
                break;
            }
            IncrementString(currentPasswd);
        }

        MPI::Finalize();
    }
    std::cout << out.str();
}