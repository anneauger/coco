#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "coco.h"
#include "coco_problem.c"
#include "suite_bbob_legacy_code.c"
#include "transform_vars_affine.c"
#include "transform_vars_shift.c"
#include "transform_obj_shift.c"

static double f_griewank_rosenbrock_raw(const double *x, const size_t number_of_variables) {

  size_t i = 0;
  double tmp = 0;
  double result;

  /* Computation core */
  result = 0.0;
  for (i = 0; i < number_of_variables - 1; ++i) {
    const double c1 = x[i] * x[i] - x[i + 1];
    const double c2 = 1.0 - x[i];
    tmp = 100.0 * c1 * c1 + c2 * c2;
    result += tmp / 4000. - cos(tmp);
  }
  result = 10. + 10. * result / (double) (number_of_variables - 1);

  return result;
}

static void f_griewank_rosenbrock_evaluate(coco_problem_t *self, const double *x, double *y) {
  assert(self->number_of_objectives == 1);
  y[0] = f_griewank_rosenbrock_raw(x, self->number_of_variables);
}

static coco_problem_t *f_griewank_rosenbrock_allocate(const size_t number_of_variables) {

  coco_problem_t *problem = coco_problem_allocate_from_scalars("Griewank Rosenbrock function",
      f_griewank_rosenbrock_evaluate, NULL, number_of_variables, -5.0, 5.0, 1);
  coco_problem_set_id(problem, "%s_d%02lu", "griewank_rosenbrock", number_of_variables);

  /* Compute best solution */
  f_griewank_rosenbrock_evaluate(problem, problem->best_parameter, problem->best_value);
  return problem;
}

static coco_problem_t *f_griewank_rosenbrock_bbob_problem_allocate(const size_t function,
                                                                   const size_t dimension,
                                                                   const size_t instance,
                                                                   const long rseed,
                                                                   const char *problem_id_template,
                                                                   const char *problem_name_template) {
  double fopt;
  coco_problem_t *problem = NULL;
  size_t i, j;

  double *M = coco_allocate_vector(dimension * dimension);
  double *b = coco_allocate_vector(dimension);
  double *shift = coco_allocate_vector(dimension);
  double scales, **rot1;

  fopt = bbob2009_compute_fopt(function, instance);
  for (i = 0; i < dimension; ++i) {
    shift[i] = -0.5;
  }

  rot1 = bbob2009_allocate_matrix(dimension, dimension);
  bbob2009_compute_rotation(rot1, rseed, dimension);
  scales = coco_max_double(1., sqrt((double) dimension) / 8.);
  for (i = 0; i < dimension; ++i) {
    for (j = 0; j < dimension; ++j) {
      rot1[i][j] *= scales;
    }
  }

  problem = f_griewank_rosenbrock_allocate(dimension);
  problem = f_transform_obj_shift(problem, fopt);
  problem = f_transform_vars_shift(problem, shift, 0);
  bbob2009_copy_rotation_matrix(rot1, M, b, dimension);
  problem = f_transform_vars_affine(problem, M, b, dimension);

  bbob2009_free_matrix(rot1, dimension);

  coco_problem_set_id(problem, problem_id_template, function, instance, dimension);
  coco_problem_set_name(problem, problem_name_template, function, instance, dimension);
  coco_problem_set_type(problem, "4-multi-modal");

  coco_free_memory(M);
  coco_free_memory(b);
  coco_free_memory(shift);
  return problem;
}
