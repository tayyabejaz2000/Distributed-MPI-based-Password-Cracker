#pragma once

#include <mpi.h>

/*
@brief A Struct containing information for a MPI Job used in brute-force cracking password 
*/
struct MPIJobData
{
    //Encyption Method + Salt
    char setting[20] = {};
    //Original Hash value from /etc/shadow
    char originalHash[200] = {};

    //Brute-Force Starting password
    char startingPasswd[9] = {};
    //Brute-Force Ending Password
    char endingPasswd[9] = {};

    /*
    @brief Returns custom MPI::Datatype of MPIJobData struct for use with MPI API Calls
    @return MPI::Datatype for MPIJobData struct
    */
    static inline MPI::Datatype MPIDataType()
    {
        int sizes[] = {
            sizeof(MPIJobData::setting),
            sizeof(MPIJobData::originalHash),
            sizeof(MPIJobData::startingPasswd),
            sizeof(MPIJobData::endingPasswd),
        };
        MPI::Aint offsets[] = {
            offsetof(MPIJobData, setting),
            offsetof(MPIJobData, originalHash),
            offsetof(MPIJobData, startingPasswd),
            offsetof(MPIJobData, endingPasswd),
        };

        MPI::Datatype datatypes[] = {
            MPI::CHAR,
            MPI::CHAR,
            MPI::CHAR,
            MPI::CHAR,
        };
        auto type = MPI::Datatype::Create_struct(4, sizes, offsets, datatypes);
        type.Commit();
        return type;
    }
};
