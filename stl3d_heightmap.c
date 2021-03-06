#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "stl3d_lib.h"

#define _GEN_SMALLER_BOTTOM_MESH

stl_error_t
stl_from_heightmap_uchar_file(
	char *filename,
	stl_origin_t origin,
	unsigned int cols,
	unsigned int rows,
	double scale_pct,
	double base_height,
	double units_per_pixel,
	stl_t **stl
	)
{
	stl_error_t   error = STL_SUCCESS;
	int           res = 0;
	FILE          *fp = NULL;
	unsigned char *vals = NULL;

	if(NULL == filename)
	{
		error = STL_LOG_ERR(STL_ERROR_INVALID_ARG);
	}

	if(STL_SUCCESS == error)
	{
		fp = fopen(filename, "rb");
		if(NULL == fp)
		{
			error = STL_LOG_ERR(STL_ERROR_IO_ERROR);
		}
	}

	if(STL_SUCCESS == error)
	{
		vals = (unsigned char *)malloc(cols * rows * sizeof(vals[0]));
		if(NULL == vals)
		{
			error = STL_LOG_ERR(STL_ERROR_MEMORY_ERROR);
		}
	}

	if(STL_SUCCESS == error)
	{
		res = fread(vals, 1, cols * rows * sizeof(vals[0]), fp);

		if(res != cols * rows * sizeof(vals[0]))
		{
			error = STL_LOG_ERR(STL_ERROR_IO_ERROR);
		}
	}

	if(STL_SUCCESS == error)
	{
		error = stl_from_heightmap_uchar(vals, origin, cols, rows, scale_pct, base_height, units_per_pixel, stl);
	}

	if(NULL != fp)
	{
		fclose(fp);
		fp = NULL;
	}

	if(NULL != vals)
	{
		free(vals);
		vals = NULL;
	}

	return STL_LOG_ERR(error);
}

stl_error_t
stl_from_heightmap_uchar(
	const unsigned char *vals,
	stl_origin_t origin,
	unsigned int cols,
	unsigned int rows,
	double scale_pct,
	double base_height,
	double units_per_pixel,
	stl_t **stl
	)
{
	stl_error_t  error = STL_SUCCESS;
	unsigned int i = 0;
	double       *vals_double = NULL;

	if((NULL == vals) || (0 == cols) || (0 == rows))
	{
		error = STL_LOG_ERR(STL_ERROR_INVALID_ARG);
	}

	vals_double = (double *)malloc(cols * rows * sizeof(vals_double[0]));
	if(NULL == vals_double)
	{
		error = STL_LOG_ERR(STL_ERROR_MEMORY_ERROR);
	}

	if(STL_SUCCESS == error)
	{
		for(i = 0; i < cols * rows; i++)
		{
			vals_double[i] = vals[i];
		}

		error = stl_from_heightmap_double(vals_double, origin, cols, rows, scale_pct, base_height, units_per_pixel, stl);
	}

	if(NULL != vals_double)
	{
		free(vals_double);
		vals_double = NULL;
	}

	return STL_LOG_ERR(error);
}

stl_error_t
stl_from_heightmap_char(
	const signed char *vals,
	stl_origin_t origin,
	unsigned int cols,
	unsigned int rows,
	double scale_pct,
	double base_height,
	double units_per_pixel,
	stl_t **stl
	)
{
	stl_error_t  error = STL_SUCCESS;
	unsigned int i = 0;
	double       *vals_double = NULL;

	if((NULL == vals) || (0 == cols) || (0 == rows))
	{
		error = STL_LOG_ERR(STL_ERROR_INVALID_ARG);
	}

	vals_double = (double *)malloc(cols * rows * sizeof(vals_double[0]));
	if(NULL == vals_double)
	{
		error = STL_LOG_ERR(STL_ERROR_MEMORY_ERROR);
	}

	if(STL_SUCCESS == error)
	{
		for(i = 0; i < cols * rows; i++)
		{
			vals_double[i] = vals[i];
		}

		error = stl_from_heightmap_double(vals_double, origin, cols, rows, scale_pct, base_height, units_per_pixel, stl);
	}

	if(NULL != vals_double)
	{
		free(vals_double);
		vals_double = NULL;
	}

	return STL_LOG_ERR(error);
}

/* Make an STL object from an array of double values
 */
stl_error_t
stl_from_heightmap_double(
	const double *vals,
	stl_origin_t origin,
	unsigned int cols,
	unsigned int rows,
	double scale_pct,
	double base_height,
	double units_per_pixel,
	stl_t **newstl
	)
{
	stl_error_t  error = STL_SUCCESS;
	unsigned int fascet_count = 0;
	unsigned int i = 0;
	unsigned int c = 0;
	unsigned int r = 0;
	unsigned int f = 0;
	double       min_z = 0.0;
	double       min_z_scaled = 0.0;
	double const **hmap = NULL;
	const double *tmp = NULL;
	stl_t        *stl = NULL;
#ifdef _GEN_SMALLER_BOTTOM_MESH
	double       center_x = 0.0;
	double       center_y = 0.0;
#endif

	if((NULL == vals) || (cols < 2) || (rows < 2) || (scale_pct <= 0.0) || (units_per_pixel <= 0.0) || (NULL == newstl))
	{
		error = STL_LOG_ERR(STL_ERROR_INVALID_ARG);
	}

	/* Create the array used for the 2d array representation */
	if(STL_SUCCESS == error)
	{
		hmap = (double const **)malloc(rows * sizeof(hmap[0]));
		if(NULL == hmap)
		{
			error = STL_LOG_ERR(STL_ERROR_MEMORY_ERROR);
		}
	}

	/* Set up a 2D array of the array of heightmap vals, to make referenceing
	 * them simpler later
	 */
	if(STL_SUCCESS == error)
	{
		for(r = 0; r < rows; r++)
		{
			hmap[r] = &vals[r * cols];
		}
	}

	/* If the origin is top left then we need to swap the rows.
	 * STL origin is bottom left
	 */
	if((STL_SUCCESS == error) && (STL_ORIGIN_TOP_LEFT == origin))
	{
		for(i = 0, r = rows - 1; i < rows / 2; i++, r--)
		{
			tmp = hmap[i];
			hmap[i] = hmap[r];
			hmap[r] = tmp;
		}
	}

	if(STL_SUCCESS == error)
	{
#if 0
		unsigned int count1 = 0;
		unsigned int count2 = 0;
#endif

		/* Calculate how many fascents we will need for this STL file
		 */

		/* This many for the top mesh */
		fascet_count = (cols - 1) * (rows - 1) * 2;

		/* This many for the bottom mesh
		 *
		 * TODO - This can be reduced from:
		 * ((cols - 1) * (rows - 1) * 2) down to ((cols - 1) * 2) + ((rows - 1) * 2)
		 *
		 * By taking a center point of the grid and connecting all outter points to it. So for
		 * a 375x462 map the bottom mesh would go from 344828 triangles down to 1670.
		 */
#ifdef _GEN_SMALLER_BOTTOM_MESH
		fascet_count += ((cols - 1) * 2) + ((rows - 1) * 2);
#else
		fascet_count += (cols - 1) * (rows - 1) * 2;
#endif

		/* Figuring out the number of triangles for both mesh sizes */
#if 0
		count1 = (cols - 1) * (rows - 1) * 2;
		count2 = ((cols - 1) * 2) + ((rows - 1) * 2);
#endif

		/* This many for the top side */
		fascet_count += (cols - 1) * 2;

		/* This many for the bottom side */
		fascet_count += (cols - 1) * 2;

		/* This many for the left side */
		fascet_count += (rows - 1) * 2;

		/* This many for the right side */
		fascet_count += (rows - 1) * 2;

		error = stl_new(&stl, fascet_count);
	}

	/*  1    2    3    4    5
	 *  6    7    8    9    10
	 * 11   12   13   14    15
	 * 16   17   18   19    20
	 * 21   22   23   24    25
	 */
	if(STL_SUCCESS == error)
	{
		/* Find the lowest point in the heightmap */
		min_z = hmap[0][0];

		for(r = 0; r < rows; r++)
		{
			for(c = 0; c < cols; c++)
			{
				if(hmap[r][c] < min_z)
				{
					min_z = hmap[r][c];
				}
			}
		}

		min_z_scaled = min_z * (scale_pct / 100.0);

		f = 0;

		/* Generate the top mesh */
		for(r = 0; r < rows - 1; r++)
		{
			for(c = 0; c < cols - 1; c++)
			{
				/* Triangle 1
				 */
				/* point 1 */
				stl->facets[f].verticies[0].x = (float)(c * units_per_pixel);
				stl->facets[f].verticies[0].y = (float)(r * units_per_pixel);
				stl->facets[f].verticies[0].z = (float)((hmap[r][c] * (scale_pct / 100.0)) + base_height);

				/* point 2 */
				stl->facets[f].verticies[1].x = (float)((c + 1) * units_per_pixel);
				stl->facets[f].verticies[1].y = (float)(r * units_per_pixel);
				stl->facets[f].verticies[1].z = (float)((hmap[r][c + 1] * (scale_pct / 100.0)) + base_height);

				/* point 6 */
				stl->facets[f].verticies[2].x = (float)(c * units_per_pixel);
				stl->facets[f].verticies[2].y = (float)((r + 1) * units_per_pixel);
				stl->facets[f].verticies[2].z = (float)((hmap[r + 1][c] * (scale_pct / 100.0)) + base_height);

				stl_gen_normal_vector(stl->facets[f].verticies, &stl->facets[f].normal);

				/* Triangle 2
				 */
				/* point 2 */
				stl->facets[f+1].verticies[0].x = (float)((c + 1) * units_per_pixel);
				stl->facets[f+1].verticies[0].y = (float)(r * units_per_pixel);
				stl->facets[f+1].verticies[0].z = (float)((hmap[r][c + 1] * (scale_pct / 100.0)) + base_height);

				/* point 7 */
				stl->facets[f+1].verticies[1].x = (float)((c + 1) * units_per_pixel);
				stl->facets[f+1].verticies[1].y = (float)((r + 1) * units_per_pixel);
				stl->facets[f+1].verticies[1].z = (float)((hmap[r + 1][c + 1] * (scale_pct / 100.0)) + base_height);

				/* point 6 */
				stl->facets[f+1].verticies[2].x = (float)(c * units_per_pixel);
				stl->facets[f+1].verticies[2].y = (float)((r + 1) * units_per_pixel);
				stl->facets[f+1].verticies[2].z = (float)((hmap[r + 1][c] * (scale_pct / 100.0)) + base_height);

				stl_gen_normal_vector(stl->facets[f+1].verticies, &stl->facets[f+1].normal);

				f+=2;
			}
		}

		/* Generate the bottom mesh */
#ifdef _GEN_SMALLER_BOTTOM_MESH

		center_x = ((cols - 1) / 2) * units_per_pixel;
		center_y = ((rows - 1) / 2) * units_per_pixel;

		/* Generate left and right side triangles */
		for(r = 0; r < rows - 1; r++)
		{
			/* Left triangle
			 */

			/* point 1 */
			stl->facets[f].verticies[0].x = (float)(0 * units_per_pixel);
			stl->facets[f].verticies[0].y = (float)(r * units_per_pixel);
			stl->facets[f].verticies[0].z = (float)(min_z_scaled - base_height);

			/* point 6 */
			stl->facets[f].verticies[1].x = (float)(0 * units_per_pixel);
			stl->facets[f].verticies[1].y = (float)((r + 1) * units_per_pixel);
			stl->facets[f].verticies[1].z = (float)(min_z_scaled - base_height);

			/* point center */
			stl->facets[f].verticies[2].x = (float)center_x;
			stl->facets[f].verticies[2].y = (float)center_y;
			stl->facets[f].verticies[2].z = (float)(min_z_scaled - base_height);

			stl_gen_normal_vector(stl->facets[f].verticies, &stl->facets[f].normal);

			/* Right triangle
			 */

			/* point 10 */
			stl->facets[f+1].verticies[0].x = (float)((cols - 1) * units_per_pixel);
			stl->facets[f+1].verticies[0].y = (float)((r + 1) * units_per_pixel);
			stl->facets[f+1].verticies[0].z = (float)(min_z_scaled - base_height);

			/* point 5 */
			stl->facets[f+1].verticies[1].x = (float)((cols - 1) * units_per_pixel);
			stl->facets[f+1].verticies[1].y = (float)(r * units_per_pixel);
			stl->facets[f+1].verticies[1].z = (float)(min_z_scaled - base_height);

			/* point center */
			stl->facets[f+1].verticies[2].x = (float)center_x;
			stl->facets[f+1].verticies[2].y = (float)center_y;
			stl->facets[f+1].verticies[2].z = (float)(min_z_scaled - base_height);

			stl_gen_normal_vector(stl->facets[f+1].verticies, &stl->facets[f+1].normal);

			f += 2;
		}

		/* Generate top and bottom triangles */
		for(c = 0; c < cols - 1; c++)
		{
			/* Top triangle
			 */
			/* point 2 */
			stl->facets[f].verticies[0].x = (float)((c + 1) * units_per_pixel);
			stl->facets[f].verticies[0].y = (float)(0 * units_per_pixel);
			stl->facets[f].verticies[0].z = (float)(min_z_scaled - base_height);

			/* point 1 */
			stl->facets[f].verticies[1].x = (float)(c * units_per_pixel);
			stl->facets[f].verticies[1].y = (float)(0 * units_per_pixel);
			stl->facets[f].verticies[1].z = (float)(min_z_scaled - base_height);

			/* point center */
			stl->facets[f].verticies[2].x = (float)center_x;
			stl->facets[f].verticies[2].y = (float)center_y;
			stl->facets[f].verticies[2].z = (float)(min_z_scaled - base_height);

			stl_gen_normal_vector(stl->facets[f].verticies, &stl->facets[f].normal);

			/* Bottom triangle
			 */

			/* point 21 */
			stl->facets[f+1].verticies[0].x = (float)(c * units_per_pixel);
			stl->facets[f+1].verticies[0].y = (float)((rows - 1) * units_per_pixel);
			stl->facets[f+1].verticies[0].z = (float)(min_z_scaled - base_height);

			/* point 22 */
			stl->facets[f+1].verticies[1].x = (float)((c + 1) * units_per_pixel);
			stl->facets[f+1].verticies[1].y = (float)((rows - 1) * units_per_pixel);
			stl->facets[f+1].verticies[1].z = (float)(min_z_scaled - base_height);

			/* point center */
			stl->facets[f+1].verticies[2].x = (float)center_x;
			stl->facets[f+1].verticies[2].y = (float)center_y;
			stl->facets[f+1].verticies[2].z = (float)(min_z_scaled - base_height);

			stl_gen_normal_vector(stl->facets[f+1].verticies, &stl->facets[f+1].normal);

			f += 2;
		}
#else
		for(c = 0; c < cols - 1; c++)
		{
			for(r = 0; r < rows - 1; r++)
			{
				/* Triangle 1
				 */
				/* point 1 */
				stl->facets[f].verticies[0].x = (float)(c * units_per_pixel);
				stl->facets[f].verticies[0].y = (float)(r * units_per_pixel);
				stl->facets[f].verticies[0].z = (float)(min_z_scaled - base_height);

				/* point 6 */
				stl->facets[f].verticies[1].x = (float)(c * units_per_pixel);
				stl->facets[f].verticies[1].y = (float)((r + 1) * units_per_pixel);
				stl->facets[f].verticies[1].z = (float)(min_z_scaled - base_height);

				/* point 2 */
				stl->facets[f].verticies[2].x = (float)((c + 1) * units_per_pixel);
				stl->facets[f].verticies[2].y = (float)(r * units_per_pixel);
				stl->facets[f].verticies[2].z = (float)(min_z_scaled - base_height);


				/* TODO - generate unit vector */

				/* Triangle 2
				 */
				/* point 2 */
				stl->facets[f+1].verticies[0].x = (float)((c + 1) * units_per_pixel);
				stl->facets[f+1].verticies[0].y = (float)(r * units_per_pixel);
				stl->facets[f+1].verticies[0].z = (float)(min_z_scaled - base_height);

				/* point 6 */
				stl->facets[f+1].verticies[1].x = (float)(c * units_per_pixel);
				stl->facets[f+1].verticies[1].y = (float)((r + 1) * units_per_pixel);
				stl->facets[f+1].verticies[1].z = (float)(min_z_scaled - base_height);

				/* point 7 */
				stl->facets[f+1].verticies[2].x = (float)((c + 1) * units_per_pixel);
				stl->facets[f+1].verticies[2].y = (float)((r + 1) * units_per_pixel);
				stl->facets[f+1].verticies[2].z = (float)(min_z_scaled - base_height);

				/* TODO - generate unit vector */

				f+= 2;
			}
		}
#endif

		/* Generate the top side mesh */
		for(c = 0; c < cols - 1; c++)
		{
			/* Triangle 1
			 */
			/* point 1 top */
			stl->facets[f].verticies[0].x = (float)(c * units_per_pixel);
			stl->facets[f].verticies[0].y = (float)(0 * units_per_pixel);
			stl->facets[f].verticies[0].z = (float)((hmap[0][c] * (scale_pct / 100.0)) + base_height);

			/* point 1 bottom */
			stl->facets[f].verticies[1].x = (float)(c * units_per_pixel);
			stl->facets[f].verticies[1].y = (float)(0 * units_per_pixel);
			stl->facets[f].verticies[1].z = (float)(min_z_scaled - base_height);

			/* point 2 top */
			stl->facets[f].verticies[2].x = (float)((c + 1) * units_per_pixel);
			stl->facets[f].verticies[2].y = (float)(0 * units_per_pixel);
			stl->facets[f].verticies[2].z = (float)((hmap[0][c + 1] * (scale_pct / 100.0)) + base_height);

			stl_gen_normal_vector(stl->facets[f].verticies, &stl->facets[f].normal);

			/* Triangle 2
			 */
			/* point 2 top */
			stl->facets[f+1].verticies[0].x = (float)((c + 1) * units_per_pixel);
			stl->facets[f+1].verticies[0].y = (float)(0 * units_per_pixel);
			stl->facets[f+1].verticies[0].z = (float)((hmap[0][c + 1] * (scale_pct / 100.0)) + base_height);

			/* point 1 bottom */
			stl->facets[f+1].verticies[1].x = (float)(c * units_per_pixel);
			stl->facets[f+1].verticies[1].y = (float)(0 * units_per_pixel);
			stl->facets[f+1].verticies[1].z = (float)(min_z_scaled - base_height);

			/* point 2 bottom */
			stl->facets[f+1].verticies[2].x = (float)((c + 1) * units_per_pixel);
			stl->facets[f+1].verticies[2].y = (float)(0 * units_per_pixel);
			stl->facets[f+1].verticies[2].z = (float)(min_z_scaled - base_height);

			stl_gen_normal_vector(stl->facets[f+1].verticies, &stl->facets[f+1].normal);

			f+= 2;
		}

		/* Generate the bottom side mesh */
		for(c = 0; c < cols - 1; c++)
		{
			/* Triangle 1
			 */
			/* point 21 top */
			stl->facets[f].verticies[0].x = (float)(c * units_per_pixel);
			stl->facets[f].verticies[0].y = (float)((rows - 1) * units_per_pixel);
			stl->facets[f].verticies[0].z = (float)((hmap[rows - 1][c] * (scale_pct / 100.0)) + base_height);

			/* point 22 top */
			stl->facets[f].verticies[1].x = (float)((c + 1) * units_per_pixel);
			stl->facets[f].verticies[1].y = (float)((rows - 1) * units_per_pixel);
			stl->facets[f].verticies[1].z = (float)((hmap[rows - 1][c + 1] * (scale_pct / 100.0)) + base_height);

			/* point 21 bottom */
			stl->facets[f].verticies[2].x = (float)(c * units_per_pixel);
			stl->facets[f].verticies[2].y = (float)((rows - 1) * units_per_pixel);
			stl->facets[f].verticies[2].z = (float)(min_z_scaled - base_height);

			stl_gen_normal_vector(stl->facets[f].verticies, &stl->facets[f].normal);

			/* Triangle 2
			 */
			/* point 22 top */
			stl->facets[f+1].verticies[0].x = (float)((c + 1) * units_per_pixel);
			stl->facets[f+1].verticies[0].y = (float)((rows - 1) * units_per_pixel);
			stl->facets[f+1].verticies[0].z = (float)((hmap[rows - 1][c + 1] * (scale_pct / 100.0)) + base_height);

			/* point 22 bottom */
			stl->facets[f+1].verticies[1].x = (float)((c + 1) * units_per_pixel);
			stl->facets[f+1].verticies[1].y = (float)((rows - 1) * units_per_pixel);
			stl->facets[f+1].verticies[1].z = (float)(min_z_scaled - base_height);

			/* point 21 bottom */
			stl->facets[f+1].verticies[2].x = (float)(c * units_per_pixel);
			stl->facets[f+1].verticies[2].y = (float)((rows - 1) * units_per_pixel);
			stl->facets[f+1].verticies[2].z = (float)(min_z_scaled - base_height);

			stl_gen_normal_vector(stl->facets[f+1].verticies, &stl->facets[f+1].normal);

			f+= 2;
		}

		/* Generate the left side mesh */
		for(r = 0; r < rows - 1; r++)
		{
			/* Triangle 1
			 */
			/* point 1 top */
			stl->facets[f].verticies[0].x = (float)(0 * units_per_pixel);  /* 0 -> c */
			stl->facets[f].verticies[0].y = (float)(r * units_per_pixel);
			stl->facets[f].verticies[0].z = (float)((hmap[r][0] * (scale_pct / 100.0)) + base_height);  /* 0 -> c */

			/* point 6 top */
			stl->facets[f].verticies[1].x = (float)(0 * units_per_pixel);  /* 0 -> c */
			stl->facets[f].verticies[1].y = (float)((r + 1) * units_per_pixel);
			stl->facets[f].verticies[1].z = (float)((hmap[r + 1][0] * (scale_pct / 100.0)) + base_height);  /* 0 -> c */

			/* point 1 bottom */
			stl->facets[f].verticies[2].x = (float)(0 * units_per_pixel);  /* 0 -> c */
			stl->facets[f].verticies[2].y = (float)(r * units_per_pixel);
			stl->facets[f].verticies[2].z = (float)(min_z_scaled - base_height);

			stl_gen_normal_vector(stl->facets[f].verticies, &stl->facets[f].normal);

			/* Triangle 2
			 */
			/* point 6 top */
			stl->facets[f+1].verticies[0].x = (float)(0 * units_per_pixel);  /* 0 -> c */
			stl->facets[f+1].verticies[0].y = (float)((r + 1) * units_per_pixel);
			stl->facets[f+1].verticies[0].z = (float)((hmap[r + 1][0] * (scale_pct / 100.0)) + base_height);  /* 0 -> c */

			/* point 6 bottom */
			stl->facets[f+1].verticies[1].x = (float)(0 * units_per_pixel);  /* 0 -> c */
			stl->facets[f+1].verticies[1].y = (float)((r + 1) * units_per_pixel);
			stl->facets[f+1].verticies[1].z = (float)(min_z_scaled - base_height);

			/* point 1 bottom */
			stl->facets[f+1].verticies[2].x = (float)(0 * units_per_pixel);  /* 0 -> c */
			stl->facets[f+1].verticies[2].y = (float)(r * units_per_pixel);
			stl->facets[f+1].verticies[2].z = (float)(min_z_scaled - base_height);

			stl_gen_normal_vector(stl->facets[f+1].verticies, &stl->facets[f+1].normal);

			f+= 2;
		}

		/* Generate the right side mesh */
		for(r = 0; r < rows - 1; r++)
		{
			/* Triangle 1
			 */
			/* point 5 top */
			stl->facets[f].verticies[0].x = (float)((cols - 1) * units_per_pixel);
			stl->facets[f].verticies[0].y = (float)(r * units_per_pixel);
			stl->facets[f].verticies[0].z = (float)((hmap[r][cols - 1] * (scale_pct / 100.0)) + base_height);

			/* point 5 bottom */
			stl->facets[f].verticies[1].x = (float)((cols - 1) * units_per_pixel);
			stl->facets[f].verticies[1].y = (float)(r * units_per_pixel);
			stl->facets[f].verticies[1].z = (float)(min_z_scaled - base_height);

			/* point 10 top */
			stl->facets[f].verticies[2].x = (float)((cols - 1) * units_per_pixel);
			stl->facets[f].verticies[2].y = (float)((r + 1) * units_per_pixel);
			stl->facets[f].verticies[2].z = (float)((hmap[r + 1][cols - 1] * (scale_pct / 100.0)) + base_height);

			stl_gen_normal_vector(stl->facets[f].verticies, &stl->facets[f].normal);

			/* Triangle 2
			 */
			/* point 10 top */
			stl->facets[f+1].verticies[0].x = (float)((cols - 1) * units_per_pixel);
			stl->facets[f+1].verticies[0].y = (float)((r + 1) * units_per_pixel);
			stl->facets[f+1].verticies[0].z = (float)((hmap[r + 1][cols - 1] * (scale_pct / 100.0)) + base_height);

			/* point 5 bottom */
			stl->facets[f+1].verticies[1].x = (float)((cols - 1) * units_per_pixel);
			stl->facets[f+1].verticies[1].y = (float)(r * units_per_pixel);
			stl->facets[f+1].verticies[1].z = (float)(min_z_scaled - base_height);

			/* point 10 bottom */
			stl->facets[f+1].verticies[2].x = (float)((cols - 1) * units_per_pixel);
			stl->facets[f+1].verticies[2].y = (float)((r + 1) * units_per_pixel);
			stl->facets[f+1].verticies[2].z = (float)(min_z_scaled - base_height);

			stl_gen_normal_vector(stl->facets[f+1].verticies, &stl->facets[f+1].normal);

			f+= 2;
		}
	}

	/* Cleanup */
	if(NULL != hmap)
	{
		free((void *)hmap);
		hmap = NULL;
	}

	if(STL_SUCCESS != error)
	{
		stl_free(stl);
		stl = NULL;
	}
	else
	{
		*newstl = stl;
	}

	return STL_LOG_ERR(error);
}
