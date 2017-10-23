
# This file is part of the CLBlast project. The project is licensed under Apache Version 2.0. This file follows the
# PEP8 Python style guide and uses a max-width of 120 characters per line.
#
# Author(s):
#   Cedric Nugteren <www.cedricnugteren.nl>

import sys


def get_best_results(database):
    """Retrieves the results with the lowest execution times"""
    sections_best = []
    for section in database["sections"]:
        section_best = {}

        # Stores all the section's meta data
        for attribute in section.keys():
            if attribute != "results":
                section_best[attribute] = section[attribute]

        # Find the best result
        parameters_best = None
        time_best = sys.float_info.max
        for result in section["results"]:
            if result["time"] < time_best:
                time_best = result["time"]
                parameters_best = result["parameters"]

        # Stores the best result
        section_best["results"] = [{"time": time_best, "parameters": parameters_best}]
        sections_best.append(section_best)

    return {"sections": sections_best}


def get_relative_bests(name, common_results, common_parameters, verbose=False):
    """Retrieves the parameters with the relative best execution time over different devices"""

    # Helper function
    def argmax(iterable):
        return max(enumerate(iterable), key=lambda x: x[1])[0]

    # Computes the sum of the execution times over the different devices
    performance_sums = []
    for parameters in common_parameters:
        performance_sum = sum([r["relative_performance"] for r in common_results if r["parameters"] == parameters])
        performance_sums.append(performance_sum)

    # Retrieves the entry with the highest performance
    best_index = argmax(performance_sums)
    best_performance = performance_sums[best_index]
    best_parameters = common_parameters[best_index]

    # Completed, report and return the results
    if verbose:
        print("[database] " + str(name) + " with performance " + str(best_performance))
    return best_parameters
