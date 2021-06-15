#include <bits/stdc++.h>

#include <mpi.h>
#include <omp.h>

#include <crypt.h>

#include "MPIJobData.hpp"

#define MASTER_RANK 0

using namespace std::literals;

namespace
{
    std::stringstream out;
}

/*
@brief Increments input string by 1 alphabet Only lower-case alphabetical string
@param old Input String
@return true if addition overflowed else false i.e. the string size was increased
*/
bool IncrementString(std::string &old)
{
    auto carry = true;
    auto it = old.rbegin();
    while (carry && it != old.rend())
    {
        ++(*it);
        if (*it > 'z')
            carry = true, *it = 'a';
        else
            carry = false;
        ++it;
    }
    if (carry)
        old.insert(old.begin(), 'a');
    return carry;
}

/*
@brief Convertes Base-10 integer to baseN alphabet string
@param base10 Base-10 Integer
@param baseN Base to convert to (Default 26)
@return The resulting baseN string
*/
std::string GetAlphabetBaseNString(uint64_t base10, uint8_t baseN = 26u)
{
    if (baseN > 26)
        throw std::runtime_error("Alphabetical Number system have max base of 26");
    std::string value;
    while (base10 < static_cast<uint64_t>(-1))
    {
        auto mod = static_cast<char>((base10 % baseN) + 'a');
        base10 = (base10 / baseN) - 1;
        value.push_back(mod);
    }
    return std::string(value.rbegin(), value.rend());
}

/*
@brief Generates same sized (size) Partitions
@param size No of partitions
@return Partitions
*/
std::vector<std::pair<uint64_t, uint64_t>> GetPartitions(std::size_t size)
{
    static const auto max = static_cast<uint64_t>(std::pow(26, 8) +
                                                  std::pow(26, 7) +
                                                  std::pow(26, 6) +
                                                  std::pow(26, 5) +
                                                  std::pow(26, 4) +
                                                  std::pow(26, 3) +
                                                  std::pow(26, 2) +
                                                  std::pow(25, 1));
    auto partitionSize = max / size;
    auto partitions = std::vector<std::pair<uint64_t, uint64_t>>((max % size) == 0 ? size : size + 1);
    auto i = 0ul;
    for (auto &partition : partitions)
        partition = {i, std::clamp(i + partitionSize, 0ul, max)},
        i += partitionSize;
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
    auto partitions = GetPartitions(numProcs);

    auto jobs = std::vector<MPIJobData>();
    auto job = MPIJobData{};

    //Create Data for each MPI Job
    for (auto &&[start, end] : partitions)
    {
        std::copy(setting.begin(), setting.end(), job.setting);
        std::copy(hash.begin(), hash.end(), job.originalHash);
        auto startPasswd = GetAlphabetBaseNString(start, 26);
        std::copy(startPasswd.begin(), startPasswd.end(), job.startingPasswd);
        auto endingPasswd = GetAlphabetBaseNString(end, 26);
        std::copy(endingPasswd.begin(), endingPasswd.end(), job.endingPasswd);
        jobs.push_back(job);
    }

    return jobs;
}

///TODO: Add Documentation
void Bruteforce(const MPIJobData &data, uint16_t rank, uint16_t size)
{
    auto stop = false;
#pragma omp parallel num_threads(2) shared(stop)
    {
        auto num_thread = omp_get_thread_num();
        if (num_thread == 0)
        {
            auto currentPasswd = std::string(data.startingPasswd);
            auto endingPasswd = std::string(data.endingPasswd);
            //Brute force each password until found
            while (currentPasswd != endingPasswd && !stop)
            {
                //Find Hash
                auto hash = std::string_view(crypt(currentPasswd.data(), data.setting));
                if (hash == data.originalHash)
                {
                    out << "Found Password: " << currentPasswd << '\n';
                    stop = true;
                    break;
                }
                //Increment String by 1 alphabet
                IncrementString(currentPasswd);
            }
            //Tell MASTER_RANK this job is terminating
            MPI::COMM_WORLD.Send(&stop, 1, MPI::BOOL, MASTER_RANK, MPI::ANY_TAG);
        }
        else if (num_thread == 1)
        {
            auto check = false;
            if (rank == MASTER_RANK)
            {
                //Num of processes message recieved from
                auto numProcs = 0u;
                while (!stop && numProcs < size)
                {
                    MPI::COMM_WORLD.Recv(static_cast<void *>(&check), 1, MPI::BOOL, MPI::ANY_SOURCE, MPI::ANY_TAG);
                    stop = check;
                    numProcs++;
                }
                //Tell all processes to stop
                stop = true;
                for (auto i = 0; i < size; ++i)
                {
                    if (i == MASTER_RANK)
                        continue;
                    MPI::COMM_WORLD.Send(static_cast<void *>(&stop), 1, MPI::BOOL, i, MPI::ANY_TAG);
                }
            }
            else
            {
                while (!stop)
                {
                    MPI::COMM_WORLD.Recv(static_cast<void *>(&check), 1, MPI::BOOL, MPI::ANY_SOURCE, MPI::ANY_TAG);
                    stop = check;
                }
                out << "Rank: " << rank << " Terminating, Password not found.\n";
            }
        }
    }
}
int main(int argc, char **argv)
{
    //Print bool as (true/false)
    out << std::boolalpha;

    MPI::Init(argc, argv);
    if (MPI::Is_initialized())
    {
        auto size = static_cast<uint32_t>(MPI::COMM_WORLD.Get_size());
        auto rank = static_cast<uint32_t>(MPI::COMM_WORLD.Get_rank());

        auto data = std::vector<MPIJobData>();
        auto type = MPIJobData::MPIDataType();

        //Only MASTER_RANK initializes data
        //and scatters it to all ranks
        if (rank == MASTER_RANK)
            data = Initialization("mpiuser", size);

        //Local data for each MPI Job
        MPIJobData localData;

        //Scatter Data
        for (auto i = 0u; i < size; ++i)
        {
            if (i == MASTER_RANK)
                continue;
            else if (rank == MASTER_RANK)
                MPI::COMM_WORLD.Send(&data[i], 1, type, i, MPI::ANY_TAG);
            else
                MPI::COMM_WORLD.Recv(&localData, 1, type, MPI::ANY_SOURCE, MPI::ANY_TAG);
        }

        if (rank == MASTER_RANK)
        {
            if (size < data.size())
            {
                localData = data.back();
                Bruteforce(localData, rank, size);
            }
        }
        else
            Bruteforce(localData, rank, size);
        MPI::Finalize();
    }
    //Flush any local MPI Job data to stdout
    std::cout << out.str();
}