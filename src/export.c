#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hpdf.h>
#include <libudev.h>
#include "core.h"
#include "measure.h"

#define USB_MOUNT_POINT "/media/usb"

void error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data) {
    printf("[PDF ERREUR] Error_no=%04X, detail_no=%u\n", (unsigned int)error_no, (unsigned int)detail_no);
}

int generate_mrtd_pdf(const char *pdf_path, const char *asset_name, const char *graph_png_path, const MrtdPoint *results, int results_count) {
    
    // 1. Initialisation de LibHaru
    HPDF_Doc pdf = HPDF_New(error_handler, NULL);
    if (!pdf) {
        printf("[PDF ERREUR] Impossible d'initialiser LibHaru.\n");
        return 0;
    }

    // 2. Création de la page A4
    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);
    
    // 3. Chargement des polices
    HPDF_Font font_title = HPDF_GetFont(pdf, "Helvetica-Bold", NULL);
    HPDF_Font font_text  = HPDF_GetFont(pdf, "Helvetica", NULL);

    // ==========================================
    // ECRITURE DU TEXTE
    // ==========================================
    HPDF_Page_BeginText(page);

    // Titre centré
    const char *titre = "Rapport de Mesure MRTD";
    HPDF_Page_SetFontAndSize(page, font_title, 24);
    float tw = HPDF_Page_TextWidth(page, titre);
    HPDF_Page_TextOut(page, (595.0 - tw) / 2.0, 800, titre); // 595.0 est la largeur d'une page A4

    // Sous-titre (Asset) centré
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Tested imager asset : %s", asset_name);
    HPDF_Page_SetFontAndSize(page, font_text, 14);
    tw = HPDF_Page_TextWidth(page, buffer);
    HPDF_Page_TextOut(page, (595.0 - tw) / 2.0, 760, buffer);

    // En-têtes du tableau (coordonnées manuelles)
    HPDF_Page_SetFontAndSize(page, font_text, 12);
    HPDF_Page_TextOut(page, 150, 720, "Spatial frequency (cy/mrad)");
    HPDF_Page_TextOut(page, 380, 720, "Delta T (mK)");

    // Lignes du tableau
    int y_pos = 690;
    for (int i = 0; i < results_count; i++) {
        // X=190 pour la colonne Fréquence, X=390 pour la colonne Delta
        snprintf(buffer, sizeof(buffer), "%.3f", results[i].target);
        HPDF_Page_TextOut(page, 190, y_pos, buffer);
        
        snprintf(buffer, sizeof(buffer), "%.3f", results[i].delta_t);
        HPDF_Page_TextOut(page, 390, y_pos, buffer);
        
        y_pos -= 20; // On descend de 20 pixels pour la ligne suivante
    }
    
    HPDF_Page_EndText(page);

    // ==========================================
    // INSERTION DE L'IMAGE PNG
    // ==========================================
    if (access(graph_png_path, F_OK) == 0) {
        HPDF_Image image = HPDF_LoadPngImageFromFile(pdf, graph_png_path);
        if (image) {
            // Dessin de l'image (X, Y, Largeur, Hauteur) centrée
            // (595 - 450) / 2 = 72.5
            HPDF_Page_DrawImage(page, image, 72.5, y_pos - 350, 450, 300);
        } else {
            printf("[PDF ERREUR] Impossible de charger l'image PNG.\n");
        }
    } else {
        printf("[PDF ATTENTION] Fichier PNG introuvable: %s\n", graph_png_path);
    }

    // ==========================================
    // SAUVEGARDE
    // ==========================================
    HPDF_STATUS status = HPDF_SaveToFile(pdf, pdf_path);
    HPDF_Free(pdf);

    if (status != HPDF_OK) {
        printf("[PDF ERREUR] Échec de la sauvegarde.\n");
        return 0;
    }

    return 1;
}

int is_usb_key_present() {
    struct udev *udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *entry;
    int usb_found = -1;

    // 1. Création du contexte udev
    udev = udev_new();
    if (!udev) {
        printf("Erreur : Impossible de créer le contexte udev.\n");
        return -1;
    }

    // 2. Création de l'énumérateur pour scanner les périphériques
    enumerate = udev_enumerate_new(udev);
    
    // On cherche dans le sous-système "block" (les périphériques de stockage)
    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_scan_devices(enumerate);

    // 3. Récupération de la liste des périphériques trouvés
    devices = udev_enumerate_get_list_entry(enumerate);

    // 4. Boucle sur chaque périphérique de stockage
    udev_list_entry_foreach(entry, devices) {
        const char *syspath = udev_list_entry_get_name(entry);
        struct udev_device *device = udev_device_new_from_syspath(udev, syspath);

        // On vérifie si ce périphérique de stockage a un parent qui est de type "usb_device"
        struct udev_device *parent = udev_device_get_parent_with_subsystem_devtype(device, "usb", "usb_device");
        
        if (parent) {
            // C'est un périphérique de stockage USB !
            const char *devnode = udev_device_get_devnode(device); // ex: /dev/sda
            
            // Pour ne détecter que les partitions (ex: /dev/sda1) et non le disque brut (/dev/sda),
            // on peut vérifier le DEVTYPE
            const char *devtype = udev_device_get_devtype(device);
            if (devtype && strcmp(devtype, "partition") == 0) {
                printf("Clé USB détectée : %s\n", devnode);
                usb_found = 0; // Clé USB trouvée
                udev_device_unref(device);
                break; // On a trouvé une clé, on peut sortir de la boucle
            }
        }
        udev_device_unref(device);
    }

    // 5. Libération de la mémoire
    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    return usb_found;
}

/*
 * @param base_path Le répertoire de destination (ex: "/media/usb")
 * @param filename Le nom du fichier de base (ex: "asset_test")
 * @param extension L'extension du fichier (ex: ".pdf")
 * @param output_path Le buffer de retour qui contiendra le chemin final
 * @param max_len La taille maximale du buffer de retour

    Permet de ne pas écraser de fichier déjà existant.
 */
void get_unique_filename(const char *base_path, const char *filename, const char *extension, char *output_path, size_t max_len) {
    int counter = 0;

    // Premier essai avec le nom de base : "/media/usb/asset_test.pdf"
    snprintf(output_path, max_len, "%s/%s%s", base_path, filename, extension);

    // F_OK vérifie si le fichier "existe"
    // access renvoie 0 si le fichier existe, -1 s'il n'existe pas
    while (access(output_path, F_OK) == 0) {
        counter++;
        // Si ça existe, on essaie avec _1, _2, etc.
        // ex: "/media/usb/asset_test_1.pdf"
        snprintf(output_path, max_len, "%s/%s_%d%s", base_path, filename, counter, extension);
        
        // Sécurité : éviter une boucle infinie si problème matériel
        if (counter > 999) {
            LOG_MSG("[WARNING] Limite d'incrémentation atteinte.");
            break;
        }
    }
    
    // Quand on sort du while, output_path contient un nom libre !
    LOG_MSG("[FILE] Nom unique généré : %s", output_path);
}

int copy_to_usb(const char *source_path, const char *dest_path) {
    char commande[1024];

    // On prépare la commande shell : cp "/dev/shm/pdf_result.pdf" "/media/usb/asset123_1.pdf"
    // L'utilisation des guillemets \" protège les chemins si jamais l'utilisateur a mis des espaces dans l'asset
    snprintf(commande, sizeof(commande), "cp \"%s\" \"%s\"", source_path, dest_path);

    // On exécute la commande via le shell
    int ret = system(commande);

    // system() renvoie 0 si la commande shell a réussi
    if (ret == 0) {
        LOG_MSG("[COPIE] Succès : %s -> %s", source_path, dest_path);
        return 0;
    } else {
        LOG_MSG("[COPIE] Erreur : La commande '%s' a échoué.", commande);
        return -1;
    }
}

int thread_export(AppData *app) {
    if (app == NULL) return 0;

    if (app->results_count <= 0 || app->mrtd_results == NULL) {
        LOG_MSG("Aucune donnée à exporter.");
        return -1;
    }

    const char *asset        = app->asset_name;
    const char *png          = MRTD_PNG_PATH;
    const char *tmp_pdf_path = MRTD_PDF_PATH;
    char dest_path[256] = {0};

    LOG_MSG("Check si USB monté...");

    int ret_usb = is_usb_key_present();

    if (ret_usb < 0) {
        LOG_MSG("Clé usb non détectée. Assurez-vous qu'elle est connectée.");
        g_idle_add(hmi_log_append_idle, strdup("ERROR: Clé USB non détectée. Assurez-vous qu'elle est connectée."));
        return -1;
    } 

    LOG_MSG("Ret usb : %d", ret_usb);

    LOG_MSG("Début de la génération du PDF...");

    

    int pdf_ok = generate_mrtd_pdf(tmp_pdf_path, asset, png, app->mrtd_results, app->results_count);

    if (!pdf_ok) {
        LOG_MSG("Échec de la création du PDF.");
        return -1;
    }

    LOG_MSG("Début du transfert vers USB...");
    
    get_unique_filename(USB_MOUNT_POINT, asset, ".pdf", dest_path, sizeof(dest_path));

    int ret_copy = copy_to_usb(tmp_pdf_path, dest_path);

    if (ret_copy == 0) {
        LOG_MSG("Exportation terminée avec succès !");
        g_idle_add(hmi_log_append_idle, strdup("Export successful !"));
        return 0;
    } else {
        LOG_MSG("Échec du transfert USB.");
        return -1;
    }
}

