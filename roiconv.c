/*

Copyright (c) 2021, Dominic Szablewski - https://phoboslab.org
SPDX-License-Identifier: MIT


Command line tool to convert between png <> qoi format

Requires:
	-"stb_image.h" (https://github.com/nothings/stb/blob/master/stb_image.h)
	-"stb_image_write.h" (https://github.com/nothings/stb/blob/master/stb_image_write.h)
	-"qoi.h" (https://github.com/phoboslab/qoi/blob/master/qoi.h)

Compile with: 
	gcc qoiconv.c -std=c99 -O3 -o qoiconv

*/


#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_LINEAR
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define QOI_IMPLEMENTATION
#include "roi.h"


#define STR_ENDS_WITH(S, E) (strcmp(S + strlen(S) - (sizeof(E)-1), E) == 0)

int main(int argc, char **argv) {
	if (argc < 3) {
		puts("Usage: roiconv <infile> <outfile>");
		puts("Examples:");
		puts("  roiconv input.png output.roi");
		puts("  roiconv input.roi output.png");
		exit(1);
	}

	void *pixels = NULL;
	int w, h, channels;
	if (STR_ENDS_WITH(argv[1], ".png")) {
		if(!stbi_info(argv[1], &w, &h, &channels)) {
			printf("Couldn't read header %s\n", argv[1]);
			exit(1);
		}

		// Force all odd encodings to be RGBA
		if(channels != 3) {
			channels = 4;
		}

		pixels = (void *)stbi_load(argv[1], &w, &h, NULL, channels);
	}
	else if (STR_ENDS_WITH(argv[1], ".ppm")) {
		//https://netpbm.sourceforge.net/doc/ppm.html#format
		int maxval;
		unsigned char r;
		FILE *fp=fopen(argv[1], "rb");
		if(1!=fread(&r, 1, 1, fp))
			goto ERR;
		if(r!='P')
			goto ERR;
		if(1!=fread(&r, 1, 1, fp))
			goto ERR;
		if(r!='6')
			goto ERR;
		while(1){//whitespace
			if(1!=fread(&r, 1, 1, fp))
				goto ERR;
			if(r>='0' && r<='9')
				break;
		}
		w=0;
		while(1){
			if(r>='0' && r<='9')
				w=(w*10)+(r-'0');
			else
				break;
			if(1!=fread(&r, 1, 1, fp))
				goto ERR;
		}
		while(1){//whitespace
			if(1!=fread(&r, 1, 1, fp))
				goto ERR;
			if(r>='0' && r<='9')
				break;
		}
		h=0;
		while(1){
			if(r>='0' && r<='9')
				h=(h*10)+(r-'0');
			else
				break;
			if(1!=fread(&r, 1, 1, fp))
				goto ERR;
		}
		while(1){//whitespace
			if(1!=fread(&r, 1, 1, fp))
				goto ERR;
			if(r>='0' && r<='9')
				break;
		}
		maxval=0;
		while(1){
			if(r>='0' && r<='9')
				maxval=(maxval*10)+(r-'0');
			else
				break;
			if(1!=fread(&r, 1, 1, fp))
				goto ERR;
		}
		if(maxval>255)
			goto ERR;
		if(w<=0)
			goto ERR;
		if(h<=0)
			goto ERR;
		pixels=malloc(w*h*3);
		channels=3;
		if((w*h*3)!=fread(pixels, 1, w*h*3, fp))
			goto ERR;
		fclose(fp);
		printf("Read ppm file w h maxval %d %d %d\n", w, h, maxval);
		goto NOERR;
		ERR:
		printf("Invalid or unsupported ppm file\n");
		exit(1);
	}
	else if (STR_ENDS_WITH(argv[1], ".roi")) {
		qoi_desc desc;
		pixels = qoi_read(argv[1], &desc, 0);
		channels = desc.channels;
		w = desc.width;
		h = desc.height;
	}
	NOERR:

	if (pixels == NULL) {
		printf("Couldn't load/decode %s\n", argv[1]);
		exit(1);
	}

	int encoded = 0;
	if (STR_ENDS_WITH(argv[2], ".png")) {
		encoded = stbi_write_png(argv[2], w, h, channels, pixels, 0);
	}
	else if (STR_ENDS_WITH(argv[2], ".roi")) {
		encoded = qoi_write(argv[2], pixels, &(qoi_desc){
			.width = w,
			.height = h, 
			.channels = channels,
			.colorspace = QOI_SRGB
		});
	}

	if (!encoded) {
		printf("Couldn't write/encode %s\n", argv[2]);
		exit(1);
	}

	free(pixels);
	return 0;
}
