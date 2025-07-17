#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hpdf.h>
#include <yaml.h>

// Buffers for fields
#define PAGE_MARGIN_TOP 750
#define PAGE_MARGIN_BOTTOM 50
#define LINE_HEIGHT 20
#define IMAGE_HEIGHT 150
#define IMAGE_WIDTH 200
#define SPACING 10
char title[100] = "";
char version[20] = "";

void parse_yaml(const char* filename) {
    FILE* file = fopen(filename, "r");
    size_t line_size=0;
    char* line=0;
    ssize_t line_len=0;
    while ((line_len=getline(&line, &line_size, file))>0) {
        process_line(line, line_size);
        free(line);
        line=NULL;
    }
}

void process_line(char* line, size_t line_size) {
    // bro what the fuck
}

void generate_pdf(const char* filename) {
    HPDF_Doc pdf;

    pdf = HPDF_New (NULL, NULL);
    if (!pdf) {
        printf ("ERROR: cannot create pdf object.\n");
        return 1;
  }

    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 16);

    HPDF_SaveToFile(pdf, filename);
    HPDF_Free(pdf);
// need to integrate this with the yaml
}

void add_line(HPDF_Doc pdf, HPDF_Page *page, const char* text, float *y_pos) {
    if (*y_pos < PAGE_MARGIN_BOTTOM) {
        // Page overflow: create new page
        *page = HPDF_AddPage(pdf);
        HPDF_Page_SetFontAndSize(*page, HPDF_GetFont(pdf, "Helvetica", NULL), 12);
        *y_pos = PAGE_MARGIN_TOP;
    }

    HPDF_Page_BeginText(*page);
    HPDF_Page_MoveTextPos(*page, 50, *y_pos);
    HPDF_Page_ShowText(*page, text);
    HPDF_Page_EndText(*page);

    *y_pos -= LINE_HEIGHT;
}

void add_image(HPDF_Doc pdf, HPDF_Page *page, const char* image_path, float *y_pos) {
    if (*y_pos - IMAGE_HEIGHT < PAGE_MARGIN_BOTTOM) {
        // Page overflow: create new page
        *page = HPDF_AddPage(pdf);
        HPDF_Page_SetFontAndSize(*page, HPDF_GetFont(pdf, "Helvetica", NULL), 12);
        *y_pos = PAGE_MARGIN_TOP;
    }

    HPDF_Page_BeginText(*page);
    HPDF_Image image = HPDF_LoadJpegImageFromFile(pdf, image_path);
    HPDF_Page_DrawImage(page, image, 50, *y_pos - IMAGE_HEIGHT, IMAGE_WIDTH, IMAGE_HEIGHT);
    y_pos -= IMAGE_HEIGHT + SPACING;
    HPDF_Page_EndText(*page);

    *y_pos -= LINE_HEIGHT;
}

void main() {
    // Enter the name of your YAML file (.yml or .yaml) here
    parse_yaml("input.yaml");
    // Enter the name of your output file (.pdf) here
    generate_pdf("output.pdf");
}
