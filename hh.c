#include <hpdf.h>

int main() {
    HPDF_Doc pdf = HPDF_New(NULL, NULL);
    if (!pdf) {
        printf("Failed to create PDF\n");
        return 1;
    }

    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 16);

    HPDF_Page_BeginText(page);
    HPDF_Page_MoveTextPos(page, 50, 750);
    HPDF_Page_ShowText(page, "Hello from Haru!");
    HPDF_Page_EndText(page);

    // Load JPEG image
    HPDF_Image image = HPDF_LoadJpegImageFromFile(pdf, "images/hello.jpg");
    HPDF_Page_DrawImage(page, image, 50, 550, 200, 100);

    HPDF_SaveToFile(pdf, "output.pdf");
    HPDF_Free(pdf);

    printf("Generated PDF with libHaru!\n");
    return 0;
}