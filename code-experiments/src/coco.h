/**
 * All public functions, constants and variables are defined in this file. It is the authoritative
 * reference, if any function deviates from the documented behavior it is considered a bug.
 */
 
#ifndef __COCO_H__
#define __COCO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Definitions of some 32 and 64-bit types (used by the random number generator) */
#ifdef _MSC_VER
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

/* Include definition for NAN among other things */
#include <math.h>
#ifndef NAN
/** @note To be used only if undefined by the included headers */
#define NAN 8.8888e88
#endif

/* COCO's own pi constant. Simplifies the case, when the value of pi changes. */
static const double coco_pi = 3.14159265358979323846;
static const double coco_two_pi = 2.0 * 3.14159265358979323846;

/* Allow for not more than 1000 instances */
#define COCO_MAX_INSTANCES 1000

/** Logging level type.
 *
 * Possible values:
 * COCO_ERROR: only error messages are output
 * COCO_WARNING: error and warning messages are output
 * COCO_INFO: error, warning and info messages are output
 * COCO_DEBUG: error, warning, info and debug messages are output
 */
typedef enum {
  COCO_ERROR, COCO_WARNING, COCO_INFO, COCO_DEBUG
} coco_log_level_type_e;

/**
 * Function to signal a fatal error.
 */
void coco_error(const char *message, ...);

/**
 * Function to warn about error conditions.
 */
void coco_warning(const char *message, ...);

/**
 * Function to output some information.
 */
void coco_info(const char *message, ...);

/**
 * Function to output detailed information that can be used for debugging.
 */
void coco_debug(const char *message, ...);

struct coco_problem;
typedef struct coco_problem coco_problem_t;
typedef void (*coco_optimizer_t)(coco_problem_t *problem);

struct coco_observer;
typedef struct coco_observer coco_observer_t;

struct coco_suite;
typedef struct coco_suite coco_suite_t;

/**
 * Evaluate the COCO problem represented by ${self} with the
 * parameter settings ${x} and save the result in ${y}.
 *
 * @note Both x and y must point to correctly sized allocated memory
 * regions.
 */
void coco_evaluate_function(coco_problem_t *self, const double *x, double *y);

/**
 * Evaluate the constraints of the COCO problem represented by
 * ${self} with the parameter settings ${x} and save the result in
 * ${y}.
 *
 * @note ${x} and ${y} are expected to be of the correct sizes.
 */
void coco_evaluate_constraint(coco_problem_t *self, const double *x, double *y);

/**
 * Recommend ${number_of_solutions} parameter settings (stored in
 * ${x}) as the current best guess solutions to the problem ${self}.
 *
 * @note ${number_of_solutions} is expected to be larger than 1 only
 * if coco_problem_get_number_of_objectives(self) is larger than 1. 
 */
void coco_recommend_solutions(coco_problem_t *self, const double *x, size_t number_of_solutions);

/**
 * Free the COCO problem represented by ${self}.
 */
void coco_problem_free(coco_problem_t *self);

/**
 * Return the name of a COCO problem.
 *
 * @note Do not modify the returned string! If you free the problem,
 * the returned pointer becomes invalid. When in doubt, strdup() the
 * returned value.
 *
 * @see coco_strdup()
 */
const char *coco_problem_get_name(const coco_problem_t *self);

/**
 * Return the ID of the COCO problem ${self}. The ID is guaranteed to
 * contain only characters in the set [a-z0-9_-]. It should therefore
 * be safe to use the ID to construct filenames or other identifiers.
 *
 * Each problem ID should be unique within each benchmark suite. 
 *
 * @note Do not modify the returned string! If you free the problem,
 * the returned pointer becomes invalid. When in doubt, strdup() the
 * returned value.
 *
 * @see coco_strdup
 */
const char *coco_problem_get_id(const coco_problem_t *self);

/**
 * Return the dimension of a COCO problem.
 */
size_t coco_problem_get_dimension(const coco_problem_t *self);

/**
 * Return the number of objectives of a COCO problem.
 */
size_t coco_problem_get_number_of_objectives(const coco_problem_t *self);

/**
 * Return the number of constraints of a COCO problem.
 */
size_t coco_problem_get_number_of_constraints(const coco_problem_t *self);

/**
 * Number of evaluations done on problem ${self}. 
 * Tentative and yet versatile. 
 */
long coco_problem_get_evaluations(coco_problem_t *self);
double coco_problem_get_best_observed_fvalue1(const coco_problem_t *self);

/**
 * Return target value for first objective. Values below are not
 * relevant in the performance assessment. 
 *
 * This function breaks the black-box property: the returned 
 * value is not meant to be used by the optimization algorithm 
 * other than for final termination. 


 */
double coco_problem_get_final_target_fvalue1(const coco_problem_t *self);

/**
 * tentative getters for region of interest
 */
const double *coco_problem_get_smallest_values_of_interest(const coco_problem_t *self);
const double *coco_problem_get_largest_values_of_interest(const coco_problem_t *self);

size_t coco_problem_get_suite_dep_index(coco_problem_t *self);

/**
 * Return an initial solution, i.e. a feasible variable setting, to the
 * problem.
 *
 * By default, the center of the problems region of interest
 * is the initial solution.
 *
 * @see coco_problem_get_smallest_values_of_interest() and
 *coco_problem_get_largest_values_of_interest()
 */
void coco_problem_get_initial_solution(const coco_problem_t *self, double *initial_solution);

coco_suite_t *coco_suite(const char *suite_name, const char *suite_instance, const char *suite_options);

void coco_suite_free(coco_suite_t *suite);

coco_problem_t *coco_suite_get_next_problem(coco_suite_t *suite, coco_observer_t *observer);

coco_problem_t *coco_suite_get_problem(coco_suite_t *suite, size_t problem_index);

size_t coco_suite_encode_problem_index(coco_suite_t *suite,
                                       const size_t function_idx,
                                       const size_t dimension_idx,
                                       const size_t instance_idx);

void coco_suite_decode_problem_index(coco_suite_t *suite,
                                     const size_t problem_index,
                                     size_t *function,
                                     size_t *instance,
                                     size_t *dimension);

size_t coco_suite_get_number_of_problems(coco_suite_t *suite);

size_t coco_suite_get_function_from_function_index(coco_suite_t *suite, size_t function_idx);

size_t coco_suite_get_dimension_from_dimension_index(coco_suite_t *suite, size_t dimension_idx);

size_t coco_suite_get_instance_from_instance_index(coco_suite_t *suite, size_t instance_idx);

void coco_run_benchmark(const char *suite_name,
                        const char *suite_instance,
                        const char *suite_options,
                        const char *observer_name,
                        const char *observer_options,
                        coco_optimizer_t optimizer);

coco_observer_t *coco_observer(const char *observer_name, const char *options);
void coco_observer_free(coco_observer_t *self);
coco_problem_t *coco_problem_add_observer(coco_problem_t *problem, coco_observer_t *observer);
/**************************************************************************
 * Random number generator
 */

struct coco_random_state;
typedef struct coco_random_state coco_random_state_t;

/**
 * Create a new random number stream using ${seed} and return its state.
 */
coco_random_state_t *coco_random_new(uint32_t seed);

/**
 * Free all memory associated with the RNG state.
 */
void coco_random_free(coco_random_state_t *state);

/**
 * Return one uniform [0, 1) random value from the random number
 * generator associated with ${state}.
 */
double coco_random_uniform(coco_random_state_t *state);

/**
 * Generate an approximately normal random number.
 *
 * Instead of using the (expensive) polar method, we may cheat and
 * abuse the central limit theorem. The sum of 12 uniform RVs has mean
 * 6, variance 1 and is approximately normal. Subtract 6 and you get
 * an approximately N(0, 1) random number.
 */
double coco_random_normal(coco_random_state_t *state);

/* Memory management routines.
 *
 * Their implementation may never fail. They either return a valid
 * pointer or terminate the program.
 */
void *coco_allocate_memory(const size_t size);
double *coco_allocate_vector(const size_t size);
void coco_free_memory(void *data);

/**
 * Create a duplicate of a string and return a pointer to
 * it. The caller is responsible for releasing the allocated memory
 * using coco_free_memory().
 *
 * @see coco_free_memory()
 */
char *coco_strdup(const char *string);

/**
 * Formatted string duplication. Optional arguments are
 * used like in sprintf.
 */
char *coco_strdupf(const char *str, ...);

int coco_remove_directory(const char *path);

#ifdef __cplusplus
}
#endif
#endif
