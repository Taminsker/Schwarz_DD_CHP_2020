#ifndef SRC_LYRA_PARTITION_LYRA_FORCUSTOM_HPP
#define SRC_LYRA_PARTITION_LYRA_FORCUSTOM_HPP

#include <iomanip>
#include <iostream>
#include <random>
#include <string>

#include "../core/core.hpp"

template <typename T>
error_t
MakeCustom (Mesh<T> * mesh, std::string filename, ul_t nparts)
{
    ul_t numPoints = mesh->GetNumberOfPoints ();

    for (ul_t idPoint = 0; idPoint < numPoints; ++idPoint)
        mesh->GetPoint (idPoint)->procsidx = {nparts + 1};

    filename = filename + ".custom." + std::to_string (nparts) + ".log";

    std::random_device              rd;                    // obtain a random number from hardware
    std::mt19937                    gen (rd ());           // seed the generator
    std::uniform_int_distribution<> distr (0, numPoints);  // define the range

    std::vector<ul_t> seedsid (nparts, 0);

    for (ul_t id = 0; id < nparts; ++id)
    {
        seedsid [id]                            = distr (gen);
        mesh->GetPoint (seedsid [id])->procsidx = {id};
        ERROR << "seed " << id << " " << mesh->GetPoint (seedsid [id])->x << " " << mesh->GetPoint (seedsid [id])->y << " " << seedsid [id] << ENDLINE;
    }

    std::vector<ul_t> card (nparts, 1);
    // card [0]       = numPoints + 1 - nparts;
    ul_t targetmin = numPoints / nparts - numPoints % nparts - 1;
    ul_t targetmax = numPoints / nparts + numPoints % nparts + 1;

    STATUS << "target-min " << targetmin << ENDLINE;
    STATUS << "target-max " << targetmax << ENDLINE;

    bool needtorecompute = true;
    int  iter            = 0;

    while (needtorecompute)
    {
        std::vector<ul_t> base (numPoints, 0);
        for (ul_t idPoint = 0; idPoint < numPoints; ++idPoint)
            base [idPoint] = mesh->GetPoint (idPoint)->procsidx [0];

        for (ul_t idProc = 0; idProc < nparts; ++idProc)
        {
            if (card [idProc] < targetmax)
            {
                for (ul_t idPoint = 0; idPoint < numPoints; ++idPoint)
                {
                    Point<T> * p = mesh->GetPoint (idPoint);

                    if (base [idPoint] == idProc)
                    {
                        for (ul_t idneighs = 0; idneighs < 4; ++idneighs)
                        {
                            Point<T> * neigh = p->neigh [idneighs];

                            if (neigh && (neigh->procsidx [0] < nparts || card [neigh->procsidx [0]] > 1))
                            {
                                card [neigh->procsidx [0]]--;
                                neigh->procsidx [0] = idProc;
                                card [idProc]++;
                            }
                        }
                    }
                }
            }
        }

        INFOS << COLOR_GREEN << "iter : " << iter << " np : " << numPoints << "  " << FLUSHLINE;

        for (ul_t idProc = 0; idProc < nparts; ++idProc)
        {
            needtorecompute = needtorecompute || (card [idProc] < targetmin || card [idProc] > targetmax);

            std::cout << "[" << COLOR_BLUE << idProc << COLOR_DEFAULT << ", #" << card [idProc] << "] " << FLUSHLINE;
        }

        std::cout << ENDLINE;

        // std::cout << " " << needtorecompute << ENDLINE;

        if (iter > 1000)
            break;

        iter++;
    }

    return EXIT_SUCCESS;
}
#endif /* SRC_LYRA_PARTITION_LYRA_FORCUSTOM_HPP */
