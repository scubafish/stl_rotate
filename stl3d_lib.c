#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "stl3d_lib.h"

int _log_err(int error, char *file, int line)
{
	if(STL_SUCCESS != error)
	{
		fprintf(stderr, "Error: %d  %s(%d)\n", error, file, line);
	}

	return error;
}

static double deg2rad(double deg)
{
	return (deg * M_PI / 180);
}

static double rad2deg(double rad)
{
	return (rad * 180 / M_PI);
}

void stl_print(stl_t *stl)
{
	int i = 0;

	if(NULL == stl)
	{
		printf("NULL stl\n");
		return;
	}

	printf("Skipping Header\n");
	printf("stl->facets_count: %d\n", stl->facets_count);

	for(i = 0; i < stl->facets_count; i++)
	{
		printf("Facet %d:\n", i+1);

		printf("   Norm: %f %f %f\n", stl->facets[i].normal.t1, stl->facets[i].normal.t2, stl->facets[i].normal.t3);
		printf("      V1  : %f %f %f\n", stl->facets[i].vertex1.t1, stl->facets[i].vertex1.t2, stl->facets[i].vertex1.t3);
		printf("      V2  : %f %f %f\n", stl->facets[i].vertex2.t1, stl->facets[i].vertex2.t2, stl->facets[i].vertex2.t3);
		printf("      V3  : %f %f %f\n", stl->facets[i].vertex3.t1, stl->facets[i].vertex3.t2, stl->facets[i].vertex3.t3);
	}
}

/* This routine packs 4 big endian bytes from buffer into a 32 bit number,
 * converting it to the platforms native endian-ness.
 */
static unsigned int stl_pack_be32(
	const unsigned char *buffer
	)
{
	return(((unsigned int)buffer[3] <<  0) |
		((unsigned int)buffer[2] <<  8) |
		((unsigned int)buffer[1] << 16) |
		((unsigned int)buffer[0] << 24) );
}

/* This routine packs 4 little endian bytes from buffer into a 32 bit number,
 * converting it to the platforms native endian-ness.
 */
static unsigned int stl_pack_le32(
	const unsigned char *buffer
	)
{
	return(((unsigned int)buffer[3] <<  24) |
		((unsigned int)buffer[2] << 16) |
		((unsigned int)buffer[1] <<  8) |
		((unsigned int)buffer[0] <<  0) );
}

/* This routine packs 2 little endian bytes from buffer into a 16 bit number,
 * converting it to the platforms native endian-ness.
 */
static unsigned short stl_pack_le16(
	const unsigned char *buffer
	)
{
	return(((unsigned short)buffer[1] <<  8) | ((unsigned short)buffer[0] <<  0));
}


/* Takes a 32 bit integer in the platform's native endianness and converts
 * it to a 4 character array in little endian format suitable for writing to disc.
 */
static stl_error_t stl_unpack_le32(const unsigned int val, unsigned char *buffer)
{
	stl_error_t error = STL_SUCCESS;

	if(NULL == buffer)
	{
		error = STL_LOG_ERR(STL_ERROR_INVALID_ARG);
	}

	if(STL_SUCCESS == error)
	{
		buffer[0] = (unsigned char)((val >> 0) & 0xFF);
		buffer[1] = (unsigned char)((val >> 8) & 0xFF);
		buffer[2] = (unsigned char)((val >> 16) & 0xFF);
		buffer[3] = (unsigned char)((val >> 24) & 0xFF);
	}

	return STL_LOG_ERR(error);
}

/* Takes a 16 bit integer in the platform's native endianness and converts
 * it to a 2 character array in little endian format suitable for writing to disc.
 */
static stl_error_t stl_unpack_le16(const unsigned short val, unsigned char *buffer)
{
	stl_error_t error = STL_SUCCESS;

	if(NULL == buffer)
	{
		error = STL_LOG_ERR(STL_ERROR_INVALID_ARG);
	}

	if(STL_SUCCESS == error)
	{
		buffer[0] = (unsigned char)((val >> 0) & 0xFF);
		buffer[1] = (unsigned char)((val >> 8) & 0xFF);
	}

	return STL_LOG_ERR(error);
}

void stl_free(stl_t *stl)
{
	if(NULL == stl)
	{
		return;
	}

	if(NULL != stl->facets)
	{
		free(stl->facets);
		stl->facets = NULL;
	}

	free(stl);
}

static stl_error_t stl_read_next_triplet(FILE *fp, triplet_t *triplet)
{
	stl_error_t error = STL_SUCCESS;
	int         res = 0;
	float       vals[3];

	if((NULL == fp) || (NULL == triplet))
	{
		error = STL_LOG_ERR(STL_ERROR);
	}

	if(STL_SUCCESS == error)
	{
		res = fread(vals, 1, sizeof(vals), fp);
		if(sizeof(vals) != res)
		{
			error = STL_LOG_ERR(STL_ERROR);
		}
	}

	if(STL_SUCCESS == error)
	{
		triplet->t1 = vals[0];
		triplet->t2 = vals[1];
		triplet->t3 = vals[2];
	}

	return STL_LOG_ERR(error);
}

static stl_error_t stl_read_next_facet(FILE *fp, facet_t *facet)
{
	stl_error_t   error = STL_SUCCESS;
	int           res = 0;
	unsigned char buffer[48];
	float         vals[3];
	unsigned char uint16_bytes[2];

	if((NULL == fp) || (NULL == facet))
	{
		error = STL_LOG_ERR(STL_ERROR);
	}

	if(STL_SUCCESS == error)
	{
		error = stl_read_next_triplet(fp, &(facet->normal));
	}

	if(STL_SUCCESS == error)
	{
		error = stl_read_next_triplet(fp, &(facet->vertex1));
	}

	if(STL_SUCCESS == error)
	{
		error = stl_read_next_triplet(fp, &(facet->vertex2));
	}

	if(STL_SUCCESS == error)
	{
		error = stl_read_next_triplet(fp, &(facet->vertex3));
	}

	if(STL_SUCCESS == error)
	{
		res = fread(uint16_bytes, 1, sizeof(uint16_bytes), fp);
		if(sizeof(uint16_bytes) != res)
		{
			error = STL_LOG_ERR(STL_ERROR);
		}
	}

	if(STL_SUCCESS == error)
	{
		facet->abc = stl_pack_le16(uint16_bytes);
	}

	return STL_LOG_ERR(error);
}

stl_error_t stl_read_file(char *input_file, stl_t **stl_new)
{
	stl_error_t   error = STL_SUCCESS;
	FILE          *fp;
	int           i = 0;
	int           res = 0;
	unsigned char uint32_bytes[4];
	stl_t         *stl = NULL;

	if((NULL == input_file) || (NULL == stl_new))
	{
		return STL_LOG_ERR(STL_ERROR);
	}

	if(STL_SUCCESS == error)
	{
		fp = fopen(input_file, "rb");
		if(NULL == fp)
		{
			error = STL_LOG_ERR(STL_ERROR);
		}
	}

	if(STL_SUCCESS == error)
	{
		stl = (stl_t *)malloc(sizeof(*stl));
		if(NULL == stl)
		{
			error = STL_LOG_ERR(STL_ERROR_MEMORY_ERROR);
		}
	}

	if(STL_SUCCESS == error)
	{
		memset(stl, 0x00, sizeof(*stl));

		res = fread(stl->header, 1, STL_HEADER_SIZE, fp);
		if(STL_HEADER_SIZE != res)
		{
			error = STL_LOG_ERR(STL_ERROR);
		}
	}

	if(STL_SUCCESS == error)
	{
		/* Make sure we are not dealing with an ASCII STL file. Not supported yet.
		 * ASCII STL files start with "solid" at the start of the file.
		 */
		if(memcmp(stl->header, "solid", strlen("solid")) == 0)
		{
			error = STL_LOG_ERR(STL_ERROR_UNSUPPORTED);
		}
	}

	if(STL_SUCCESS == error)
	{
		res = fread(uint32_bytes, 1, sizeof(uint32_bytes), fp);
		if(sizeof(uint32_bytes) != res)
		{
			error = STL_LOG_ERR(STL_ERROR);
		}
	}

	if(STL_SUCCESS == error)
	{
		stl->facets_count = stl_pack_le32(uint32_bytes);

		stl->facets = (facet_t *)malloc(stl->facets_count * sizeof(facet_t));
		if(NULL == stl->facets)
		{
			error = STL_LOG_ERR(STL_ERROR_MEMORY_ERROR);
		}
		else
		{
			memset(stl->facets, 0x00, sizeof(stl->facets[0]));
		}
	}

	if(STL_SUCCESS == error)
	{
		for(i = 0; i < stl->facets_count; i++)
		{
			error = stl_read_next_facet(fp, &(stl->facets[i]));
			if(STL_SUCCESS != error)
			{
				break;
			}
		}		
	}

	/* Cleanup */
	if(STL_SUCCESS != error)
	{
		stl_free(stl);
		stl = NULL;
	}
	else
	{
		*stl_new = stl;
	}

	if(NULL != fp)
	{
		fclose(fp);
		fp = NULL;
	}

	return STL_LOG_ERR(error);
}

static stl_error_t stl_write_next_triplet(FILE *fp, triplet_t *triplet)
{
	stl_error_t error = STL_SUCCESS;
	int         res = 0;
	float       vals[3];

	if((NULL == fp) || (NULL == triplet))
	{
		error = STL_LOG_ERR(STL_ERROR);
	}

	if(STL_SUCCESS == error)
	{
		vals[0] = triplet->t1;
		vals[1] = triplet->t2;
		vals[2] = triplet->t3;

		res = fwrite(vals, 1, sizeof(vals), fp);
		if(sizeof(vals) != res)
		{
			error = STL_LOG_ERR(STL_ERROR_IO_ERROR);
		}
	}

	return STL_LOG_ERR(error);

}

static stl_error_t stl_write_next_facet(FILE *fp, facet_t *facet)
{
	stl_error_t   error = STL_SUCCESS;
	int           res = 0;
	float         vals[3];
	unsigned char uint16_bytes[2];

	if((NULL == fp) || (NULL == facet))
	{
		error = STL_LOG_ERR(STL_ERROR);
	}

	if(STL_SUCCESS == error)
	{
		error = stl_write_next_triplet(fp, &(facet->normal));
	}

	if(STL_SUCCESS == error)
	{
		error = stl_write_next_triplet(fp, &(facet->vertex1));
	}

	if(STL_SUCCESS == error)
	{
		error = stl_write_next_triplet(fp, &(facet->vertex2));
	}

	if(STL_SUCCESS == error)
	{
		error = stl_write_next_triplet(fp, &(facet->vertex3));
	}

	if(STL_SUCCESS == error)
	{
		error = stl_unpack_le16(facet->abc, uint16_bytes);
	}

	if(STL_SUCCESS == error)
	{
		res = fwrite(uint16_bytes, 1, sizeof(uint16_bytes), fp);
		if(sizeof(uint16_bytes) != res)
		{
			error = STL_LOG_ERR(STL_ERROR);
		}
	}

	return STL_LOG_ERR(error);
}

stl_error_t stl_write_file(char *output_file, stl_t *stl)
{
	stl_error_t  error = STL_SUCCESS;
	int           res = 0;
	int           i = 0;
	FILE          *fp = NULL;
	unsigned char buffer[4];

	if((NULL == output_file) || (NULL == stl))
	{
		return STL_LOG_ERR(STL_ERROR);
	}

	if(STL_SUCCESS == error)
	{
		/* Check if exists. If it does, fail */
		fp = fopen(output_file, "rb");
		if(NULL != fp)
		{
			fprintf(stderr, "Error: Output file %s already exists\n", output_file);
			fclose(fp);
			fp = NULL;

			error = STL_LOG_ERR(STL_ERROR);
		}
	}

	if(STL_SUCCESS == error)
	{
		/* Now create the new file */
		fp = fopen(output_file, "wb");
		if(NULL == fp)
		{
			fprintf(stderr, "Error: Could not create Output file %s\n", output_file);

			error = STL_LOG_ERR(STL_ERROR);
		}
	}

	if(STL_SUCCESS == error)
	{
		res = fwrite(stl->header, 1, STL_HEADER_SIZE, fp);
		if(STL_HEADER_SIZE != res)
		{
			error = STL_LOG_ERR(STL_ERROR);
		}
	}

	if(STL_SUCCESS == error)
	{
		error = stl_unpack_le32(stl->facets_count, buffer);
	}

	if(STL_SUCCESS == error)
	{
		res = fwrite(buffer, 1, 4, fp);
		if(4 != res)
		{
			error = STL_LOG_ERR(STL_ERROR_IO_ERROR);
		}
	}

	if(STL_SUCCESS == error)
	{
		for(i = 0; i < stl->facets_count; i++)
		{
			error = stl_write_next_facet(fp, &(stl->facets[i]));

			if(STL_SUCCESS != error)
			{
				break;
			}
		}
	}

	if(NULL != fp)
	{
		fclose(fp);
		fp = NULL;
	}

	return STL_LOG_ERR(error);
}

static void _rot_vec(double cs, double sn, float *f1, float *f2)
{
	double px = 0.0;
	double py = 0.0;

	px = *f1 * cs - *f2 * sn;
	py = *f1 * sn + *f2 * cs;

	*f1 = (float)px;
	*f2 = (float)py;

}

static void _rot_vec_x(double cs, double sn, triplet_t *triplet)
{
	double py = 0.0;
	double pz = 0.0;

	py = (double)triplet->t2 * cs - triplet->t3 * sn;
	pz = (double)triplet->t2 * sn + triplet->t3 * cs;

	triplet->t2 = (float)py;
	triplet->t3 = (float)pz;
}


static void _rot_vec_y(double cs, double sn, triplet_t *triplet)
{
	double px = 0.0;
	double pz = 0.0;

	px = (double)triplet->t1 * cs + triplet->t3 * sn;
	pz = -(double)triplet->t1 * sn + triplet->t3 * cs;

	triplet->t1 = (float)px;
	triplet->t3 = (float)pz;
}

static void _rot_vec_z(double cs, double sn, triplet_t *triplet)
{
	double px = 0.0;
	double py = 0.0;
	
	px = (double)triplet->t1 * cs - triplet->t2 * sn;
	py = (double)triplet->t1 * sn + triplet->t2 * cs;

	triplet->t1 = (float)px;
	triplet->t2 = (float)py;
}

stl_error_t stl_rotate(stl_axis_t axis, float degrees, stl_t *stl)
{
	stl_error_t error = STL_SUCCESS;
	int         i = 0;
	double      radians = 0.0;
	double      cs = 0.0;
	double      sn = 0.0;
	double      px = 0.0;
	double      py = 0.0;
	double      pz = 0.0;

	if(NULL == stl)
	{
		return STL_LOG_ERR(STL_ERROR);
	}

	if(STL_SUCCESS == error)
	{
		radians = deg2rad(degrees);
		cs = cos(radians);
		sn = sin(radians);

		/* TODO - The elegant way to do this is to set up a rotation matrix,
		 * which will also let us rotate around any vector. Do this.
		 */
		for(i = 0; i < stl->facets_count; i++)
		{
			if(STL_AXIS_X == axis)
			{
				_rot_vec_x(cs, sn, &stl->facets[i].normal);
				_rot_vec_x(cs, sn, &stl->facets[i].vertex1);
				_rot_vec_x(cs, sn, &stl->facets[i].vertex2);
				_rot_vec_x(cs, sn, &stl->facets[i].vertex3);
			}
			else if(STL_AXIS_Y == axis)
			{
				_rot_vec_y(cs, sn, &stl->facets[i].normal);
				_rot_vec_y(cs, sn, &stl->facets[i].vertex1);
				_rot_vec_y(cs, sn, &stl->facets[i].vertex2);
				_rot_vec_y(cs, sn, &stl->facets[i].vertex3);
			}
			else
			{
				/* axis == z */
				_rot_vec_z(cs, sn, &stl->facets[i].normal);
				_rot_vec_z(cs, sn, &stl->facets[i].vertex1);
				_rot_vec_z(cs, sn, &stl->facets[i].vertex2);
				_rot_vec_z(cs, sn, &stl->facets[i].vertex3);
			}
		}
	}

	return STL_LOG_ERR(error);
}

stl_error_t stl_scale(double pct_x, double pct_y, double pct_z, stl_t *stl)
{
	stl_error_t error = STL_SUCCESS;
	int         i = 0;
	double      scale_x = pct_x / 100.0;
	double      scale_y = pct_y / 100.0;
	double      scale_z = pct_z / 100.0;

	if(NULL == stl)
	{
		error = STL_LOG_ERR(STL_ERROR_INVALID_ARG);
	}


	if(STL_SUCCESS == error)
	{
		for(i = 0; i < stl->facets_count; i++)
		{
			stl->facets[i].vertex1.t1 *= scale_x;
			stl->facets[i].vertex1.t2 *= scale_y;
			stl->facets[i].vertex1.t3 *= scale_z;

			stl->facets[i].vertex2.t1 *= scale_x;
			stl->facets[i].vertex2.t2 *= scale_y;
			stl->facets[i].vertex2.t3 *= scale_z;

			stl->facets[i].vertex3.t1 *= scale_x;
			stl->facets[i].vertex3.t2 *= scale_y;
			stl->facets[i].vertex3.t3 *= scale_z;
		}
	}

	return STL_LOG_ERR(error);
}