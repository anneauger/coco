#include <assert.h>
#include <math.h>

#include "coco.h"
#include "coco_problem.c"
#include "coco_utilities.c"
#include "suite_bbob_legacy_code.c"
#include "transform_obj_shift.c"

static double *gallagher_peaks;

typedef struct {
  long rseed;
  size_t number_of_peaks;
  double *xopt;
  double **rotation, **x_local, **arr_scales;
  double *peak_values;
  coco_free_function_t old_free_problem;
} f_gallagher_data_t;

/**
 * Comparison function used for sorting.
 */
static int f_gallagher_compare_doubles(const void *a, const void *b) {
  double temp = gallagher_peaks[*(const size_t *) a] - gallagher_peaks[*(const size_t *) b];
  if (temp > 0)
    return 1;
  else if (temp < 0)
    return -1;
  else
    return 0;
}
static double f_gallagher_raw(const double *x, const size_t number_of_variables, f_gallagher_data_t *data) {
  size_t i, j; /* Loop over dim */
  double *tmx;
  double a = 0.1;
  double tmp2, f = 0., Fadd, tmp, Fpen = 0., Ftrue = 0.;
  double fac;
  double result;

  fac = -0.5 / (double) number_of_variables;

  /* Boundary handling */
  for (i = 0; i < number_of_variables; ++i) {
    tmp = fabs(x[i]) - 5.;
    if (tmp > 0.) {
      Fpen += tmp * tmp;
    }
  }
  Fadd = Fpen;
  /* Transformation in search space */
  /* TODO: this should rather be done in f_gallagher */
  tmx = coco_allocate_vector(number_of_variables);
  for (i = 0; i < number_of_variables; i++) {
    tmx[i] = 0;
    for (j = 0; j < number_of_variables; ++j) {
      tmx[i] += data->rotation[i][j] * x[j];
    }
  }
  /* Computation core*/
  for (i = 0; i < data->number_of_peaks; ++i) {
    tmp2 = 0.;
    for (j = 0; j < number_of_variables; ++j) {
      tmp = (tmx[j] - data->x_local[j][i]);
      tmp2 += data->arr_scales[i][j] * tmp * tmp;
    }
    tmp2 = data->peak_values[i] * exp(fac * tmp2);
    f = coco_max_double(f, tmp2);
  }

  f = 10. - f;
  if (f > 0) {
    Ftrue = log(f) / a;
    Ftrue = pow(exp(Ftrue + 0.49 * (sin(Ftrue) + sin(0.79 * Ftrue))), a);
  } else if (f < 0) {
    Ftrue = log(-f) / a;
    Ftrue = -pow(exp(Ftrue + 0.49 * (sin(0.55 * Ftrue) + sin(0.31 * Ftrue))), a);
  } else
    Ftrue = f;

  Ftrue *= Ftrue;
  Ftrue += Fadd;
  result = Ftrue;
  coco_free_memory(tmx);
  return result;
}

static void f_gallagher_evaluate(coco_problem_t *self, const double *x, double *y) {
  assert(self->number_of_objectives == 1);
  y[0] = f_gallagher_raw(x, self->number_of_variables, self->data);
  assert(y[0] >= self->best_value[0]);
}

static void f_gallagher_free(coco_problem_t *self) {
  f_gallagher_data_t *data;
  data = self->data;
  coco_free_memory(data->xopt);
  coco_free_memory(data->peak_values);
  bbob2009_free_matrix(data->rotation, self->number_of_variables);
  bbob2009_free_matrix(data->x_local, self->number_of_variables);
  bbob2009_free_matrix(data->arr_scales, data->number_of_peaks);
  self->free_problem = NULL;
  coco_problem_free(self);

  if (gallagher_peaks != NULL) {
    coco_free_memory(gallagher_peaks);
    gallagher_peaks = NULL;
  }
}

/* Note: there is no separate f_gallagher_allocate() function! */

static coco_problem_t *f_gallagher_bbob_problem_allocate(const size_t function,
                                                         const size_t dimension,
                                                         const size_t instance,
                                                         const long rseed,
                                                         const size_t number_of_peaks,
                                                         const char *problem_id_template,
                                                         const char *problem_name_template) {

  f_gallagher_data_t *data;
  /* problem_name and best_parameter will be overwritten below */
  coco_problem_t *problem = coco_problem_allocate_from_scalars("Gallagher function",
      f_gallagher_evaluate, f_gallagher_free, dimension, -5.0, 5.0, 0.0);

  const size_t peaks_21 = 21;
  const size_t peaks_101 = 101;

  double fopt;
  size_t i, j, k, *rperm;
  double maxcondition = 1000.;
  /* maxcondition1 satisfies the old code and the doc but seems wrong in that it is, with very high
   * probability, not the largest condition level!!! */
  double maxcondition1 = 1000.;
  double *arrCondition;
  double fitvalues[2] = { 1.1, 9.1 };
  /* Parameters for generating local optima. In the old code, they are different in f21 and f22 */
  double b, c;

  data = coco_allocate_memory(sizeof(*data));
  /* Allocate temporary storage and space for the rotation matrices */
  data->number_of_peaks = number_of_peaks;
  data->xopt = coco_allocate_vector(dimension);
  data->rotation = bbob2009_allocate_matrix(dimension, dimension);
  data->x_local = bbob2009_allocate_matrix(dimension, number_of_peaks);
  data->arr_scales = bbob2009_allocate_matrix(number_of_peaks, dimension);

  if (number_of_peaks == peaks_101) {
    if (gallagher_peaks != NULL)
      coco_free_memory(gallagher_peaks);
    gallagher_peaks = coco_allocate_vector(peaks_101 * dimension);
    maxcondition1 = sqrt(maxcondition1);
    b = 10.;
    c = 5.;
  } else if (number_of_peaks == peaks_21) {
    if (gallagher_peaks != NULL)
      coco_free_memory(gallagher_peaks);
    gallagher_peaks = coco_allocate_vector(peaks_21 * dimension);
    b = 9.8;
    c = 4.9;
  } else {
    coco_error("f_gallagher(): '%lu' is a bad number of peaks", number_of_peaks);
  }
  data->rseed = rseed;
  bbob2009_compute_rotation(data->rotation, rseed, dimension);

  /* Initialize all the data of the inner problem */
  bbob2009_unif(gallagher_peaks, number_of_peaks - 1, data->rseed);
  rperm = (size_t *) coco_allocate_memory((number_of_peaks - 1) * sizeof(size_t));
  for (i = 0; i < number_of_peaks - 1; ++i)
    rperm[i] = i;
  qsort(rperm, number_of_peaks - 1, sizeof(size_t), f_gallagher_compare_doubles);

  /* Random permutation */
  arrCondition = coco_allocate_vector(number_of_peaks);
  arrCondition[0] = maxcondition1;
  data->peak_values = coco_allocate_vector(number_of_peaks);
  data->peak_values[0] = 10;
  for (i = 1; i < number_of_peaks; ++i) {
    arrCondition[i] = pow(maxcondition, (double) (rperm[i - 1]) / ((double) (number_of_peaks - 2)));
    data->peak_values[i] = (double) (i - 1) / (double) (number_of_peaks - 2) * (fitvalues[1] - fitvalues[0])
        + fitvalues[0];
  }
  coco_free_memory(rperm);

  rperm = (size_t *) coco_allocate_memory(dimension * sizeof(size_t));
  for (i = 0; i < number_of_peaks; ++i) {
    bbob2009_unif(gallagher_peaks, dimension, data->rseed + (long) (1000 * i));
    for (j = 0; j < dimension; ++j)
      rperm[j] = j;
    qsort(rperm, dimension, sizeof(size_t), f_gallagher_compare_doubles);
    for (j = 0; j < dimension; ++j) {
      data->arr_scales[i][j] = pow(arrCondition[i],
          ((double) rperm[j]) / ((double) (dimension - 1)) - 0.5);
    }
  }
  coco_free_memory(rperm);

  bbob2009_unif(gallagher_peaks, dimension * number_of_peaks, data->rseed);
  for (i = 0; i < dimension; ++i) {
    data->xopt[i] = 0.8 * (b * gallagher_peaks[i] - c);
    problem->best_parameter[i] = 0.8 * (b * gallagher_peaks[i] - c);
    for (j = 0; j < number_of_peaks; ++j) {
      data->x_local[i][j] = 0.;
      for (k = 0; k < dimension; ++k) {
        data->x_local[i][j] += data->rotation[i][k] * (b * gallagher_peaks[j * dimension + k] - c);
      }
      if (j == 0) {
        data->x_local[i][j] *= 0.8;
      }
    }
  }
  coco_free_memory(arrCondition);

  problem->data = data;

  /* Compute best solution */
  f_gallagher_evaluate(problem, problem->best_parameter, problem->best_value);

  fopt = bbob2009_compute_fopt(function, instance);
  problem = f_transform_obj_shift(problem, fopt);

  coco_problem_set_id(problem, problem_id_template, function, instance, dimension);
  coco_problem_set_name(problem, problem_name_template, function, instance, dimension);
  coco_problem_set_type(problem, "5-weakly-structured");

  return problem;
}
