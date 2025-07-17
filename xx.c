#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Buffers for fields
char title[100] = "";
char version[20] = "";
char endpoint[100] = "";
char summary[200] = "";
char description[1024] = "";
char image_path[256] = "";
int has_image = 0;

// Function to parse the YAML
void parse_yaml(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("YAML file open failed");
        exit(1);
    }

    char line[256];
    int in_description = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "title:"))
            sscanf(line, "  title: %[^\n]", title);
        else if (strstr(line, "version:"))
            sscanf(line, "  version: %[^\n]", version);
        else if (line[0] == '/' && strchr(line, ':'))
            sscanf(line, "  /%[^:]:", endpoint);
        else if (strstr(line, "summary:"))
            sscanf(line, "      summary: %[^\n]", summary);
        else if (strstr(line, "description:")) {
            in_description = 1;
            continue;
        } else if (in_description && line[0] == ' ') {
            strcat(description, line);

            // Detect markdown image
            char alt[100], path[200];
            if (sscanf(line, " ![%[^]]](%[^)])", alt, path) == 2) {
                strncpy(image_path, path, sizeof(image_path));
                has_image = 1;
            }
        }
    }

    fclose(file);
}

// Helper: write PDF-safe string
void write_pdf_string(FILE* f, const char* str) {
    fputc('(', f);
    while (*str) {
        if (*str == '(' || *str == ')')
            fputc('\\', f);
        fputc(*str, f);
        str++;
    }
    fputc(')', f);
}

// Read JPEG binary
unsigned char* read_jpeg(const char* path, int* size_out) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        perror("Image open failed");
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char* buffer = malloc(size);
    fread(buffer, 1, size, f);
    fclose(f);

    *size_out = size;
    return buffer;
}

// Generate PDF file
void generate_pdf(const char* filename) {
    FILE* pdf = fopen(filename, "wb");
    if (!pdf) {
        perror("PDF file open failed");
        exit(1);
    }

    // PDF header
    fputs("%PDF-1.4\n", pdf);

    long xref[10];
    int obj_count = 1;

    // 1. Catalog
    xref[obj_count++] = ftell(pdf);
    fputs("1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n", pdf);

    // 2. Pages
    xref[obj_count++] = ftell(pdf);
    fputs("2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n", pdf);

    // 3. Page
    xref[obj_count++] = ftell(pdf);
    fprintf(pdf,
        "3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
        "/Contents 4 0 R /Resources << /Font << /F1 5 0 R >>");

    if (has_image) {
        fputs(" /XObject << /Im1 6 0 R >>", pdf);
    }

    fputs(" >> >> >>\nendobj\n", pdf);

    // 4. Contents (text + image reference)
    char stream[2048];
    int y = 720;

    int written = snprintf(stream, sizeof(stream),
        "BT\n/F1 18 Tf\n72 %d Td\n", y);
    written += snprintf(stream + written, sizeof(stream) - written,
        "(");
    written += snprintf(stream + written, sizeof(stream) - written,
        "%s (v%s)) Tj\n", title, version);
    y -= 30;
    written += snprintf(stream + written, sizeof(stream) - written,
        "0 -30 Td\n(GET /%s) Tj\n", endpoint);
    y -= 30;
    written += snprintf(stream + written, sizeof(stream) - written,
        "0 -20 Td\n(Summary: %s) Tj\n", summary);
    y -= 20;
    written += snprintf(stream + written, sizeof(stream) - written,
        "0 -20 Td\n", "");

    // Print description
    char* line = strtok(description, "\n");
    while (line) {
        if (strstr(line, "![") && strstr(line, "](")) {
            // Image marker
            y -= 150;  // Space for image
            written += snprintf(stream + written, sizeof(stream) - written,
                "ET\nq\n200 0 0 100 72 %d cm\n/Im1 Do\nQ\nBT\n72 %d Td\n",
                y, y - 110);
        } else {
            written += snprintf(stream + written, sizeof(stream) - written,
                "(");
            write_pdf_string(stdout, line); // stdout for debug
            written += snprintf(stream + written, sizeof(stream) - written,
                "%s) Tj\n0 -15 Td\n", line);
            y -= 15;
        }
        line = strtok(NULL, "\n");
    }

    written += snprintf(stream + written, sizeof(stream) - written, "ET\n");

    xref[obj_count++] = ftell(pdf);
    fprintf(pdf, "4 0 obj\n<< /Length %d >>\nstream\n%s\nendstream\nendobj\n", written, stream);

    // 5. Font
    xref[obj_count++] = ftell(pdf);
    fputs("5 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>\nendobj\n", pdf);

    // 6. Image (JPEG only)
    int image_size = 0;
    unsigned char* image_data = NULL;
    if (has_image) {
        image_data = read_jpeg(image_path, &image_size);
        xref[obj_count++] = ftell(pdf);
        fprintf(pdf,
            "6 0 obj\n<< /Type /XObject /Subtype /Image\n"
            "/Width 200 /Height 100 /ColorSpace /DeviceRGB\n"
            "/BitsPerComponent 8 /Filter /DCTDecode /Length %d >>\n"
            "stream\n", image_size);
        fwrite(image_data, 1, image_size, pdf);
        fputs("\nendstream\nendobj\n", pdf);
        free(image_data);
    }

    // xref table
    long xref_start = ftell(pdf);
    fprintf(pdf, "xref\n0 %d\n0000000000 65535 f \n", obj_count);
    for (int i = 1; i < obj_count; i++) {
        fprintf(pdf, "%010ld 00000 n \n", xref[i]);
    }

    // trailer
    fprintf(pdf,
        "trailer\n<< /Size %d /Root 1 0 R >>\nstartxref\n%ld\n%%%%EOF\n",
        obj_count, xref_start);

    fclose(pdf);
    printf("PDF generated with image: output.pdf\n");
}

int main() {
    parse_yaml("input.yaml");
    generate_pdf("output.pdf");
    return 0;
}
