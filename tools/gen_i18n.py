#!/usr/bin/env python3
"""Writes resources/i18n/<locale>/pkdex.json for the SDL2 UI (pkDex 2.0).

Usage: python3 tools/gen_i18n.py [repo root]  (defaults to the repo this script is in)

One table per locale, every locale with the same keys (checked below), so a
missing translation is caught here rather than shown as a key on screen.
"""
import json
import os
import sys

ROOT = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

REGION_IDS = ["kanto", "kanto_frlg", "sinnoh", "sinnoh_arceus", "galar", "isle_armor", "crown_tundra",
              "paldea", "kitakami", "blueberry_academy", "kalos_lza", "hyperspace_lumiose"]


def regions(rows):
    # rows: id -> (name, game, sidebar, tag)
    return {rid: dict(zip(("name", "game", "sidebar", "tag"), rows[rid])) for rid in REGION_IDS}


EN = {
    "app": {"tagline": "Pokémon encyclopedia"},
    "sidebar": {"regions": "Regions", "dlc": "DLC"},
    "nav": {"settings": "Settings", "about": "About", "changelog": "Changelog"},
    "hints": {
        "quit": "Quit", "open": "Open", "page": "Page", "apply": "Apply", "done": "Done", "exit": "Exit",
        "select": "Select", "status": "Status", "bulk": "Bulk", "multi": "Multi-select", "back": "Back",
        "previous": "Previous", "next": "Next", "regular_view": "Regular view", "shiny_view": "Shiny view",
        "toggle": "Toggle", "choose": "Choose",
    },
    "common": {
        "loading": "Loading the Pokédex…", "busy": "Another task is still running", "cancel": "Cancel",
        "later": "Later", "ok": "OK", "on": "On", "off": "Off", "close": "Close", "count_pokemon": "{count} Pokémon",
    },
    "regions": regions({
        "kanto": ("Kanto", "Let's Go! Pikachu & Let's Go! Eevee", "Let's Go! Pikachu & Eevee", "LGPE"),
        "kanto_frlg": ("Kanto", "FireRed & LeafGreen", "FireRed & LeafGreen", "FRLG"),
        "sinnoh": ("Sinnoh", "Brilliant Diamond & Shining Pearl", "Brilliant Diamond & Shining Pearl", "BDSP"),
        "sinnoh_arceus": ("Sinnoh", "Legends: Arceus", "Legends: Arceus", "Legends: Arceus"),
        "galar": ("Galar", "Sword & Shield", "Sword & Shield", "SwSh"),
        "isle_armor": ("Isle of Armor", "Sword & Shield · The Isle of Armor", "The Isle of Armor", "SwSh DLC 1"),
        "crown_tundra": ("Crown Tundra", "Sword & Shield · The Crown Tundra", "The Crown Tundra", "SwSh DLC 2"),
        "paldea": ("Paldea", "Scarlet & Violet", "Scarlet & Violet", "ScVi"),
        "kitakami": ("Kitakami", "Scarlet & Violet · The Teal Mask", "The Teal Mask", "ScVi DLC 1"),
        "blueberry_academy": ("Blueberry Academy", "Scarlet & Violet · The Indigo Disk",
                              "The Indigo Disk", "ScVi DLC 2"),
        "kalos_lza": ("Kalos", "Legends: Z-A", "Legends: Z-A", "Legends: Z-A"),
        "hyperspace_lumiose": ("Hyperspace Lumiose", "Legends: Z-A · Mega Dimension", "Mega Dimension", "LZA DLC"),
    }),
    "dex": {
        "eyebrow": "Regional Pokédex", "caught": "caught", "page": "Page {page} of {pages}",
        "multi": "Multi-select · {count} selected", "empty": "No Pokémon in this region",
    },
    "states": {"regular": "Regular", "shiny": "Shiny", "alpha": "Alpha", "shiny_alpha": "Shiny Alpha"},
    "bulk": {
        "title": "Bulk actions", "subtitle": "{region} · {count} Pokémon", "mark": "Mark all", "clear": "Clear all",
        "mark_regular": "Mark all as Caught", "mark_shiny": "Mark all as Shiny", "mark_alpha": "Mark all as Alpha",
        "mark_shiny_alpha": "Mark all as Shiny Alpha", "clear_regular": "Clear all Caught",
        "clear_shiny": "Clear all Shiny", "clear_alpha": "Clear all Alpha", "clear_shiny_alpha": "Clear all Shiny Alpha",
        "confirm_title": "Update {count} Pokémon?",
        "confirm_mark_body": "Every Pokémon in {region} will be marked as {state}. This action cannot be undone.",
        "confirm_clear_body": "{state} will be cleared for every Pokémon in {region}. This action cannot be undone.",
        "apply": "Apply", "done": "{count} Pokémon updated",
    },
    "multi": {
        "on": "Multi-select: A selects, Y applies a status", "none": "Select Pokémon with A first",
        "title": "Selected Pokémon", "subtitle": "{count} selected · {region}",
    },
    "capture": {"title": "Capture status"},
    "detail": {
        "previous": "Previous", "next": "Next", "no_sprite": "No sprite",
        "numbers": "National #{national} · {region} #{regional}", "national_no": "National N°",
        "regional_no": "Regional N°", "shiny": "Shiny", "available": "Available", "locked": "Locked",
        "version": "Version exclusive", "none": "None", "evolution": "Evolution", "current": "Current",
        "no_evolution": "Does not evolve", "locations": "Locations", "unknown_location": "No known location",
        "rows": "Rows {first}–{last} of {total}",
    },
    "settings": {
        "eyebrow": "Preferences", "title": "Settings",
        "group": {"updates": "Updates", "pokemon_data": "Pokémon data", "user_interface": "User interface",
                  "language": "Language", "resources": "Resources"},
        "check_on_launch": "Check for updates on launch", "check_now": "Check for updates",
        "install_update": "Install update", "region_to_reset": "Region to reset",
        "reset_capture": "Reset capture status", "hide_footer": "Hide bottom bar", "language": "Language",
        "download_pack": "Download high-res image pack", "extract_pack": "Extract high-res image pack",
        "value": {"checking": "Checking…", "up_to_date": "Up to date", "check_failed": "Check failed",
                  "no_update": "None available", "downloaded": "Downloaded", "pack_size": "75 MB",
                  "not_downloaded": "Not downloaded"},
    },
    "reset": {
        "all_regions": "All regions", "drawer_title": "Region to reset",
        "all_title": "Reset all capture statuses?",
        "all_body": "Every Pokémon's capture status will be cleared for all regions. This action cannot be undone.",
        "region_title": "Reset {region}?",
        "region_body": "Every Pokémon's capture status will be cleared for {region}. This action cannot be undone.",
        "all_button": "Reset everything", "region_button": "Reset region",
        "success_all": "All capture statuses have been reset",
        "success_region": "Capture statuses for {region} have been reset",
        "failure": "The capture statuses could not be reset",
    },
    "language": {"title": "Language", "changed": "Language changed"},
    "quit": {"title": "Quit pkDex?", "busy": "A download is still running and will be stopped."},
    "net": {"offline": "No network connection"},
    "update": {
        "toast_available": "pkDex {version} is available: install it from Settings",
        "latest": "You are using the latest version ({version})", "check_failed": "Could not check for updates",
        "available_title": "pkDex {version} is available",
        "available_body": "You are using {current}. Download and install {version} now? pkDex restarts when it is done.",
        "install": "Install", "none": "There is no update to install",
        "screen_title": "pkDex Updater", "sub_running": "Installing v{version}", "sub_done": "Update applied",
        "sub_failed": "Update failed",
        "step_download": "Downloaded update", "step_unpack": "Unpacked update", "step_verify": "Verified the new version",
        "step_backup": "Backed up current version", "step_install": "Installed update",
        "step_cleanup": "Removed backup file",
        "success": "Update successfully applied", "restarting": "Restarting in {seconds} s",
        "failure": "The update could not be applied", "intact": "your current version was kept",
        "keep_on": "Keep the console on until the update is done",
    },
    "pack": {
        "exists_title": "Image pack already downloaded", "exists_body": "Do you want to download it again?",
        "redownload": "Download again", "downloading": "Downloading the high-res image pack",
        "downloaded_title": "Download complete",
        "downloaded_body": "The high-resolution images are ready to be extracted. Extract them now?",
        "extract": "Extract", "cancelled": "Download cancelled", "download_failed": "Download failed: {error}",
        "missing": "Download the image pack first", "extract_title": "Extract the image pack?",
        "extract_body": "This may take a while.", "extracting": "Extracting the high-res image pack",
        "extract_failed": "Extraction failed", "extracted_title": "Extraction complete",
        "extracted_body": "High-resolution images are now available. Do you want to keep the downloaded zip file?",
        "keep": "Keep", "delete": "Delete", "zip_kept": "The zip file was kept", "zip_deleted": "The zip file was deleted",
        "zip_delete_failed": "The zip file could not be deleted",
    },
    "about": {
        "eyebrow": "About", "title": "pkDex", "version": "Version {version} · by {author}",
        "description": "pkDex is a simple app to view Pokémon data offline, for the Nintendo Switch games.",
        "source_label": "Source", "source": "github.com/Insektaure/pkDex",
        "license_label": "License", "license": "GNU General Public License v2.0",
        "data_label": "Data", "data": "serebii.net · pokemondb.net · bulbapedia.bulbagarden.net",
        "logo_label": "Logo", "logo": "Generated with InvokeAI",
        "disclaimer": "pkDex is not affiliated with, endorsed by, or related to Nintendo, Game Freak, or The Pokémon "
                      "Company. Pokémon and Pokémon character names are trademarks of Nintendo. Provided as is, for "
                      "educational and informational purposes only.",
    },
    "changelog": {"eyebrow": "What's new", "title": "Changelog", "missing": "Changelog file not found."},
}

FR = {
    "app": {"tagline": "Encyclopédie Pokémon"},
    "sidebar": {"regions": "Régions", "dlc": "DLC"},
    "nav": {"settings": "Paramètres", "about": "À propos", "changelog": "Historique"},
    "hints": {
        "quit": "Quitter", "open": "Ouvrir", "page": "Page", "apply": "Appliquer", "done": "Terminé",
        "exit": "Sortir", "select": "Choisir", "status": "Statut", "bulk": "Groupé", "multi": "Multi-sélection",
        "back": "Retour", "previous": "Précédent", "next": "Suivant", "regular_view": "Vue normale",
        "shiny_view": "Vue chromatique", "toggle": "Basculer", "choose": "Choisir",
    },
    "common": {
        "loading": "Chargement du Pokédex…", "busy": "Une autre tâche est en cours", "cancel": "Annuler",
        "later": "Plus tard", "ok": "OK", "on": "Oui", "off": "Non", "close": "Fermer",
        "count_pokemon": "{count} Pokémon",
    },
    "regions": regions({
        "kanto": ("Kanto", "Let's Go, Pikachu et Let's Go, Évoli", "Let's Go, Pikachu et Évoli", "LGPE"),
        "kanto_frlg": ("Kanto", "Rouge Feu & Vert Feuille", "Rouge Feu & Vert Feuille", "RFVF"),
        "sinnoh": ("Sinnoh", "Diamant Étincelant & Perle Scintillante", "Diamant Étincelant & Perle Scintillante",
                   "DEPS"),
        "sinnoh_arceus": ("Sinnoh", "Légendes Pokémon : Arceus", "Légendes : Arceus", "Légendes Arceus"),
        "galar": ("Galar", "Épée & Bouclier", "Épée & Bouclier", "EB"),
        "isle_armor": ("Isolarmure", "Épée & Bouclier · L'Île Solitaire de l'Armure", "L'Île Solitaire de l'Armure", "EB DLC 1"),
        "crown_tundra": ("Couronneige", "Épée & Bouclier · Les Terres Enneigées de la Couronne", "Les Terres Enneigées de la Couronne",
                         "EB DLC 2"),
        "paldea": ("Paldea", "Écarlate & Violet", "Écarlate & Violet", "EV"),
        "kitakami": ("Septentria", "Écarlate & Violet · Le Masque Turquoise", "Le Masque Turquoise",
                     "EV DLC 1"),
        "blueberry_academy": ("Institut Myrtille", "Écarlate & Violet · Le Disque Indigo",
                              "Le Disque Indigo", "EV DLC 2"),
        "kalos_lza": ("Kalos", "Légendes Pokémon : Z-A", "Légendes : Z-A", "Légendes Z-A"),
        "hyperspace_lumiose": ("Extra Illumis", "Légendes : Z-A · Méga-Dimension", "Méga-Dimension", "LZA DLC"),
    }),
    "dex": {
        "eyebrow": "Pokédex régional", "caught": "capturés", "page": "Page {page} sur {pages}",
        "multi": "Multi-sélection · {count} choisis", "empty": "Aucun Pokémon dans cette région",
    },
    "states": {"regular": "Normal", "shiny": "Chromatique", "alpha": "Baron", "shiny_alpha": "Baron Chromatique"},
    "bulk": {
        "title": "Actions groupées", "subtitle": "{region} · {count} Pokémon", "mark": "Tout marquer",
        "clear": "Tout effacer",
        "mark_regular": "Tout marquer comme Capturé", "mark_shiny": "Tout marquer comme Chromatique",
        "mark_alpha": "Tout marquer comme Baron", "mark_shiny_alpha": "Tout marquer comme Baron Chromatique",
        "clear_regular": "Effacer tous les Capturés", "clear_shiny": "Effacer tous les Chromatiques",
        "clear_alpha": "Effacer tous les Barons", "clear_shiny_alpha": "Effacer tous les Barons Chromatiques",
        "confirm_title": "Modifier {count} Pokémon ?",
        "confirm_mark_body": "Tous les Pokémon de {region} seront marqués {state}. Cette action est irréversible.",
        "confirm_clear_body": "Le statut {state} sera effacé pour tous les Pokémon de {region}. Cette action est "
                              "irréversible.",
        "apply": "Appliquer", "done": "{count} Pokémon modifiés",
    },
    "multi": {
        "on": "Multi-sélection : A choisit, Y applique un statut", "none": "Choisissez d'abord des Pokémon avec A",
        "title": "Pokémon choisis", "subtitle": "{count} choisis · {region}",
    },
    "capture": {"title": "Statut de capture"},
    "detail": {
        "previous": "Précédent", "next": "Suivant", "no_sprite": "Pas d'image",
        "numbers": "National #{national} · {region} #{regional}", "national_no": "N° National",
        "regional_no": "N° Régional", "shiny": "Chromatique", "available": "Disponible", "locked": "Verrouillé",
        "version": "Exclusivité", "none": "Aucune", "evolution": "Évolution", "current": "Actuel",
        "no_evolution": "N'évolue pas", "locations": "Emplacements", "unknown_location": "Aucun emplacement connu",
        "rows": "Lignes {first}–{last} sur {total}",
    },
    "settings": {
        "eyebrow": "Préférences", "title": "Paramètres",
        "group": {"updates": "Mises à jour", "pokemon_data": "Données Pokémon", "user_interface": "Interface",
                  "language": "Langue", "resources": "Ressources"},
        "check_on_launch": "Vérifier les mises à jour au lancement", "check_now": "Vérifier les mises à jour",
        "install_update": "Installer la mise à jour", "region_to_reset": "Région à réinitialiser",
        "reset_capture": "Réinitialiser les captures", "hide_footer": "Masquer la barre du bas",
        "language": "Langue", "download_pack": "Télécharger le pack d'images HD",
        "extract_pack": "Extraire le pack d'images HD",
        "value": {"checking": "Vérification…", "up_to_date": "À jour", "check_failed": "Échec",
                  "no_update": "Aucune", "downloaded": "Téléchargé", "pack_size": "75 Mo",
                  "not_downloaded": "Non téléchargé"},
    },
    "reset": {
        "all_regions": "Toutes les régions", "drawer_title": "Région à réinitialiser",
        "all_title": "Réinitialiser toutes les captures ?",
        "all_body": "Le statut de capture de chaque Pokémon sera effacé pour toutes les régions. Cette action est "
                    "irréversible.",
        "region_title": "Réinitialiser {region} ?",
        "region_body": "Le statut de capture de chaque Pokémon sera effacé pour {region}. Cette action est "
                       "irréversible.",
        "all_button": "Tout réinitialiser", "region_button": "Réinitialiser",
        "success_all": "Toutes les captures ont été réinitialisées",
        "success_region": "Les captures de {region} ont été réinitialisées",
        "failure": "Les captures n'ont pas pu être réinitialisées",
    },
    "language": {"title": "Langue", "changed": "Langue modifiée"},
    "quit": {"title": "Quitter pkDex ?", "busy": "Un téléchargement est en cours et sera interrompu."},
    "net": {"offline": "Aucune connexion réseau"},
    "update": {
        "toast_available": "pkDex {version} est disponible : installez-le depuis les Paramètres",
        "latest": "Vous utilisez la dernière version ({version})",
        "check_failed": "Impossible de vérifier les mises à jour",
        "available_title": "pkDex {version} est disponible",
        "available_body": "Vous utilisez la {current}. Télécharger et installer la {version} maintenant ? pkDex "
                          "redémarre une fois terminé.",
        "install": "Installer", "none": "Aucune mise à jour à installer",
        "screen_title": "Mise à jour de pkDex", "sub_running": "Installation de la v{version}",
        "sub_done": "Mise à jour appliquée", "sub_failed": "Échec de la mise à jour",
        "step_download": "Mise à jour téléchargée", "step_unpack": "Mise à jour décompressée", "step_verify": "Nouvelle version vérifiée",
        "step_backup": "Version actuelle sauvegardée", "step_install": "Mise à jour installée",
        "step_cleanup": "Sauvegarde supprimée",
        "success": "Mise à jour appliquée avec succès", "restarting": "Redémarrage dans {seconds} s",
        "failure": "La mise à jour n'a pas pu être appliquée", "intact": "la version actuelle a été conservée",
        "keep_on": "Laissez la console allumée jusqu'à la fin",
    },
    "pack": {
        "exists_title": "Pack d'images déjà téléchargé", "exists_body": "Voulez-vous le télécharger à nouveau ?",
        "redownload": "Retélécharger", "downloading": "Téléchargement du pack d'images HD",
        "downloaded_title": "Téléchargement terminé",
        "downloaded_body": "Les images haute résolution sont prêtes à être extraites. Les extraire maintenant ?",
        "extract": "Extraire", "cancelled": "Téléchargement annulé", "download_failed": "Échec du téléchargement : {error}",
        "missing": "Téléchargez d'abord le pack d'images", "extract_title": "Extraire le pack d'images ?",
        "extract_body": "Cela peut prendre un moment.", "extracting": "Extraction du pack d'images HD",
        "extract_failed": "Échec de l'extraction", "extracted_title": "Extraction terminée",
        "extracted_body": "Les images haute résolution sont disponibles. Voulez-vous conserver le fichier zip "
                          "téléchargé ?",
        "keep": "Conserver", "delete": "Supprimer", "zip_kept": "Le fichier zip a été conservé",
        "zip_deleted": "Le fichier zip a été supprimé", "zip_delete_failed": "Le fichier zip n'a pas pu être supprimé",
    },
    "about": {
        "eyebrow": "À propos", "title": "pkDex", "version": "Version {version} · par {author}",
        "description": "pkDex est une application simple pour consulter les données Pokémon hors ligne, pour les "
                       "jeux Nintendo Switch.",
        "source_label": "Source", "source": "github.com/Insektaure/pkDex",
        "license_label": "Licence", "license": "GNU General Public License v2.0",
        "data_label": "Données", "data": "serebii.net · pokemondb.net · bulbapedia.bulbagarden.net",
        "logo_label": "Logo", "logo": "Généré avec InvokeAI",
        "disclaimer": "pkDex n'est ni affilié, ni approuvé, ni lié à Nintendo, Game Freak ou The Pokémon Company. "
                      "Pokémon et les noms des personnages Pokémon sont des marques de Nintendo. Fourni tel quel, à "
                      "des fins éducatives et informatives uniquement.",
    },
    "changelog": {"eyebrow": "Nouveautés", "title": "Historique", "missing": "Fichier d'historique introuvable."},
}

DE = {
    "app": {"tagline": "Pokémon-Enzyklopädie"},
    "sidebar": {"regions": "Regionen", "dlc": "DLC"},
    "nav": {"settings": "Einstellungen", "about": "Über", "changelog": "Änderungen"},
    "hints": {
        "quit": "Beenden", "open": "Öffnen", "page": "Seite", "apply": "Anwenden", "done": "Fertig",
        "exit": "Verlassen", "select": "Wählen", "status": "Status", "bulk": "Alle", "multi": "Mehrfachauswahl",
        "back": "Zurück", "previous": "Zurück", "next": "Weiter", "regular_view": "Normal",
        "shiny_view": "Schillernd", "toggle": "Umschalten", "choose": "Wählen",
    },
    "common": {
        "loading": "Pokédex wird geladen…", "busy": "Eine andere Aufgabe läuft noch", "cancel": "Abbrechen",
        "later": "Später", "ok": "OK", "on": "An", "off": "Aus", "close": "Schließen",
        "count_pokemon": "{count} Pokémon",
    },
    "regions": regions({
        "kanto": ("Kanto", "Let's Go, Pikachu! & Let's Go, Evoli!", "Let's Go, Pikachu! & Evoli!", "LGPE"),
        "kanto_frlg": ("Kanto", "Feuerrote & Blattgrüne Edition", "Feuerrot & Blattgrün", "FRLG"),
        "sinnoh": ("Sinnoh", "Strahlender Diamant & Leuchtende Perle", "Strahlender Diamant & Leuchtende Perle",
                   "SDLP"),
        "sinnoh_arceus": ("Sinnoh", "Pokémon-Legenden: Arceus", "Legenden: Arceus", "Legenden Arceus"),
        "galar": ("Galar", "Schwert & Schild", "Schwert & Schild", "SwSh"),
        "isle_armor": ("Rüstungsinsel", "Schwert & Schild · Die Insel der Abgeschiedenheit", "Die Insel der Abgeschiedenheit",
                       "SwSh DLC 1"),
        "crown_tundra": ("Kronen-Schneelande", "Schwert & Schild · Die Schneelande der Krone", "Die Schneelande der Krone",
                         "SwSh DLC 2"),
        "paldea": ("Paldea", "Karmesin & Purpur", "Karmesin & Purpur", "KaPu"),
        "kitakami": ("Kitakami", "Karmesin & Purpur · Die Türkisgrüne Maske", "Die Türkisgrüne Maske",
                     "KaPu DLC 1"),
        "blueberry_academy": ("Blaubeer-Akademie", "Karmesin & Purpur · Die Indigoblaue Scheibe",
                              "Die Indigoblaue Scheibe", "KaPu DLC 2"),
        "kalos_lza": ("Kalos", "Pokémon-Legenden: Z-A", "Legenden: Z-A", "Legenden Z-A"),
        "hyperspace_lumiose": ("Dimensions-Illumina", "Legenden: Z-A · Mega-Dimension", "Mega-Dimension",
                               "LZA DLC"),
    }),
    "dex": {
        "eyebrow": "Regionaler Pokédex", "caught": "gefangen", "page": "Seite {page} von {pages}",
        "multi": "Mehrfachauswahl · {count} gewählt", "empty": "Keine Pokémon in dieser Region",
    },
    "states": {"regular": "Normal", "shiny": "Schillernd", "alpha": "Elite", "shiny_alpha": "Schillernd Elite"},
    "bulk": {
        "title": "Massenaktionen", "subtitle": "{region} · {count} Pokémon", "mark": "Alle markieren",
        "clear": "Alle zurücksetzen",
        "mark_regular": "Alle als Gefangen markieren", "mark_shiny": "Alle als Schillernd markieren",
        "mark_alpha": "Alle als Elite markieren", "mark_shiny_alpha": "Alle als Schillernd Elite markieren",
        "clear_regular": "Alle Gefangen zurücksetzen", "clear_shiny": "Alle Schillernd zurücksetzen",
        "clear_alpha": "Alle Elite zurücksetzen", "clear_shiny_alpha": "Alle Schillernd Elite zurücksetzen",
        "confirm_title": "{count} Pokémon ändern?",
        "confirm_mark_body": "Alle Pokémon in {region} werden als {state} markiert. Dies kann nicht rückgängig "
                             "gemacht werden.",
        "confirm_clear_body": "{state} wird für alle Pokémon in {region} zurückgesetzt. Dies kann nicht rückgängig "
                              "gemacht werden.",
        "apply": "Anwenden", "done": "{count} Pokémon geändert",
    },
    "multi": {
        "on": "Mehrfachauswahl: A wählt, Y setzt einen Status", "none": "Wähle zuerst Pokémon mit A",
        "title": "Gewählte Pokémon", "subtitle": "{count} gewählt · {region}",
    },
    "capture": {"title": "Fangstatus"},
    "detail": {
        "previous": "Zurück", "next": "Weiter", "no_sprite": "Kein Bild",
        "numbers": "National #{national} · {region} #{regional}", "national_no": "National-Nr.",
        "regional_no": "Regional-Nr.", "shiny": "Schillernd", "available": "Verfügbar", "locked": "Gesperrt",
        "version": "Versions-Exklusiv", "none": "Keine", "evolution": "Entwicklung", "current": "Aktuell",
        "no_evolution": "Entwickelt sich nicht", "locations": "Fundorte", "unknown_location": "Kein bekannter Fundort",
        "rows": "Zeilen {first}–{last} von {total}",
    },
    "settings": {
        "eyebrow": "Einstellungen", "title": "Einstellungen",
        "group": {"updates": "Updates", "pokemon_data": "Pokémon-Daten", "user_interface": "Oberfläche",
                  "language": "Sprache", "resources": "Ressourcen"},
        "check_on_launch": "Beim Start nach Updates suchen", "check_now": "Nach Updates suchen",
        "install_update": "Update installieren", "region_to_reset": "Zurückzusetzende Region",
        "reset_capture": "Fangstatus zurücksetzen", "hide_footer": "Untere Leiste ausblenden",
        "language": "Sprache", "download_pack": "HD-Bilderpaket herunterladen",
        "extract_pack": "HD-Bilderpaket entpacken",
        "value": {"checking": "Suche…", "up_to_date": "Aktuell", "check_failed": "Fehlgeschlagen",
                  "no_update": "Keines", "downloaded": "Heruntergeladen", "pack_size": "75 MB",
                  "not_downloaded": "Nicht heruntergeladen"},
    },
    "reset": {
        "all_regions": "Alle Regionen", "drawer_title": "Zurückzusetzende Region",
        "all_title": "Alle Fangstatus zurücksetzen?",
        "all_body": "Der Fangstatus aller Pokémon wird für alle Regionen gelöscht. Dies kann nicht rückgängig "
                    "gemacht werden.",
        "region_title": "{region} zurücksetzen?",
        "region_body": "Der Fangstatus aller Pokémon wird für {region} gelöscht. Dies kann nicht rückgängig gemacht "
                       "werden.",
        "all_button": "Alles zurücksetzen", "region_button": "Zurücksetzen",
        "success_all": "Alle Fangstatus wurden zurückgesetzt",
        "success_region": "Die Fangstatus für {region} wurden zurückgesetzt",
        "failure": "Die Fangstatus konnten nicht zurückgesetzt werden",
    },
    "language": {"title": "Sprache", "changed": "Sprache geändert"},
    "quit": {"title": "pkDex beenden?", "busy": "Ein Download läuft noch und wird abgebrochen."},
    "net": {"offline": "Keine Netzwerkverbindung"},
    "update": {
        "toast_available": "pkDex {version} ist verfügbar: installiere es in den Einstellungen",
        "latest": "Du verwendest die neueste Version ({version})",
        "check_failed": "Suche nach Updates fehlgeschlagen",
        "available_title": "pkDex {version} ist verfügbar",
        "available_body": "Du verwendest {current}. {version} jetzt herunterladen und installieren? pkDex startet "
                          "danach neu.",
        "install": "Installieren", "none": "Kein Update zum Installieren",
        "screen_title": "pkDex-Updater", "sub_running": "v{version} wird installiert",
        "sub_done": "Update angewendet", "sub_failed": "Update fehlgeschlagen",
        "step_download": "Update heruntergeladen", "step_unpack": "Update entpackt", "step_verify": "Neue Version geprüft",
        "step_backup": "Aktuelle Version gesichert", "step_install": "Update installiert",
        "step_cleanup": "Sicherung entfernt",
        "success": "Update erfolgreich angewendet", "restarting": "Neustart in {seconds} s",
        "failure": "Das Update konnte nicht angewendet werden", "intact": "die aktuelle Version bleibt erhalten",
        "keep_on": "Lass die Konsole eingeschaltet, bis das Update fertig ist",
    },
    "pack": {
        "exists_title": "Bilderpaket bereits heruntergeladen", "exists_body": "Möchtest du es erneut herunterladen?",
        "redownload": "Erneut laden", "downloading": "HD-Bilderpaket wird heruntergeladen",
        "downloaded_title": "Download abgeschlossen",
        "downloaded_body": "Die hochauflösenden Bilder können entpackt werden. Jetzt entpacken?",
        "extract": "Entpacken", "cancelled": "Download abgebrochen", "download_failed": "Download fehlgeschlagen: {error}",
        "missing": "Lade zuerst das Bilderpaket herunter", "extract_title": "Bilderpaket entpacken?",
        "extract_body": "Dies kann eine Weile dauern.", "extracting": "HD-Bilderpaket wird entpackt",
        "extract_failed": "Entpacken fehlgeschlagen", "extracted_title": "Entpacken abgeschlossen",
        "extracted_body": "Die hochauflösenden Bilder sind jetzt verfügbar. Möchtest du die heruntergeladene "
                          "ZIP-Datei behalten?",
        "keep": "Behalten", "delete": "Löschen", "zip_kept": "Die ZIP-Datei wurde behalten",
        "zip_deleted": "Die ZIP-Datei wurde gelöscht", "zip_delete_failed": "Die ZIP-Datei konnte nicht gelöscht werden",
    },
    "about": {
        "eyebrow": "Über", "title": "pkDex", "version": "Version {version} · von {author}",
        "description": "pkDex ist eine einfache App, um Pokémon-Daten der Nintendo-Switch-Spiele offline anzusehen.",
        "source_label": "Quellcode", "source": "github.com/Insektaure/pkDex",
        "license_label": "Lizenz", "license": "GNU General Public License v2.0",
        "data_label": "Daten", "data": "serebii.net · pokemondb.net · bulbapedia.bulbagarden.net",
        "logo_label": "Logo", "logo": "Mit InvokeAI generiert",
        "disclaimer": "pkDex steht in keiner Verbindung zu Nintendo, Game Freak oder The Pokémon Company und wird von "
                      "ihnen weder unterstützt noch empfohlen. Pokémon und die Namen der Pokémon-Figuren sind Marken "
                      "von Nintendo. Bereitgestellt wie besehen, nur zu Bildungs- und Informationszwecken.",
    },
    "changelog": {"eyebrow": "Neuigkeiten", "title": "Änderungsprotokoll", "missing": "Änderungsprotokoll nicht gefunden."},
}

ES = {
    "app": {"tagline": "Enciclopedia Pokémon"},
    "sidebar": {"regions": "Regiones", "dlc": "DLC"},
    "nav": {"settings": "Ajustes", "about": "Acerca de", "changelog": "Cambios"},
    "hints": {
        "quit": "Salir", "open": "Abrir", "page": "Página", "apply": "Aplicar", "done": "Listo", "exit": "Salir",
        "select": "Elegir", "status": "Estado", "bulk": "Masivo", "multi": "Selección múltiple", "back": "Atrás",
        "previous": "Anterior", "next": "Siguiente", "regular_view": "Vista normal", "shiny_view": "Vista variocolor",
        "toggle": "Cambiar", "choose": "Elegir",
    },
    "common": {
        "loading": "Cargando la Pokédex…", "busy": "Otra tarea sigue en curso", "cancel": "Cancelar",
        "later": "Más tarde", "ok": "Aceptar", "on": "Sí", "off": "No", "close": "Cerrar",
        "count_pokemon": "{count} Pokémon",
    },
    "regions": regions({
        "kanto": ("Kanto", "Let's Go, Pikachu! y Let's Go, Eevee!", "Let's Go, Pikachu! y Eevee!", "LGPE"),
        "kanto_frlg": ("Kanto", "Rojo Fuego y Verde Hoja", "Rojo Fuego y Verde Hoja", "RFVH"),
        "sinnoh": ("Sinnoh", "Diamante Brillante y Perla Reluciente", "Diamante Brillante y Perla Reluciente", "DBPR"),
        "sinnoh_arceus": ("Sinnoh", "Leyendas Pokémon: Arceus", "Leyendas: Arceus", "Leyendas Arceus"),
        "galar": ("Galar", "Espada y Escudo", "Espada y Escudo", "EyE"),
        "isle_armor": ("Isla de la Armadura", "Espada y Escudo · La isla de la armadura", "La isla de la armadura",
                       "EyE DLC 1"),
        "crown_tundra": ("Nieves de la Corona", "Espada y Escudo · Las nieves de la corona", "Las nieves de la corona",
                         "EyE DLC 2"),
        "paldea": ("Paldea", "Escarlata y Púrpura", "Escarlata y Púrpura", "EyP"),
        "kitakami": ("Noroteo", "Escarlata y Púrpura · La máscara turquesa", "La máscara turquesa",
                     "EyP DLC 1"),
        "blueberry_academy": ("Academia Arándano", "Escarlata y Púrpura · El disco índigo",
                              "El disco índigo", "EyP DLC 2"),
        "kalos_lza": ("Kalos", "Leyendas Pokémon: Z-A", "Leyendas: Z-A", "Leyendas Z-A"),
        "hyperspace_lumiose": ("Luminalia Dimensional", "Leyendas: Z-A · Megadimensión", "Megadimensión",
                               "LZA DLC"),
    }),
    "dex": {
        "eyebrow": "Pokédex regional", "caught": "capturados", "page": "Página {page} de {pages}",
        "multi": "Selección múltiple · {count} elegidos", "empty": "No hay Pokémon en esta región",
    },
    "states": {"regular": "Normal", "shiny": "Variocolor", "alpha": "Alfa", "shiny_alpha": "Alfa Variocolor"},
    "bulk": {
        "title": "Acciones masivas", "subtitle": "{region} · {count} Pokémon", "mark": "Marcar todos",
        "clear": "Borrar todos",
        "mark_regular": "Marcar todos como Capturados", "mark_shiny": "Marcar todos como Variocolor",
        "mark_alpha": "Marcar todos como Alfa", "mark_shiny_alpha": "Marcar todos como Alfa Variocolor",
        "clear_regular": "Borrar todos los Capturados", "clear_shiny": "Borrar todos los Variocolor",
        "clear_alpha": "Borrar todos los Alfa", "clear_shiny_alpha": "Borrar todos los Alfa Variocolor",
        "confirm_title": "¿Modificar {count} Pokémon?",
        "confirm_mark_body": "Todos los Pokémon de {region} se marcarán como {state}. Esta acción no se puede deshacer.",
        "confirm_clear_body": "Se borrará {state} de todos los Pokémon de {region}. Esta acción no se puede deshacer.",
        "apply": "Aplicar", "done": "{count} Pokémon modificados",
    },
    "multi": {
        "on": "Selección múltiple: A elige, Y aplica un estado", "none": "Elige primero Pokémon con A",
        "title": "Pokémon elegidos", "subtitle": "{count} elegidos · {region}",
    },
    "capture": {"title": "Estado de captura"},
    "detail": {
        "previous": "Anterior", "next": "Siguiente", "no_sprite": "Sin imagen",
        "numbers": "Nacional #{national} · {region} #{regional}", "national_no": "N.º Nacional",
        "regional_no": "N.º Regional", "shiny": "Variocolor", "available": "Disponible", "locked": "Bloqueado",
        "version": "Exclusivo de versión", "none": "Ninguno", "evolution": "Evolución", "current": "Actual",
        "no_evolution": "No evoluciona", "locations": "Ubicaciones", "unknown_location": "Ninguna ubicación conocida",
        "rows": "Filas {first}–{last} de {total}",
    },
    "settings": {
        "eyebrow": "Preferencias", "title": "Ajustes",
        "group": {"updates": "Actualizaciones", "pokemon_data": "Datos Pokémon", "user_interface": "Interfaz",
                  "language": "Idioma", "resources": "Recursos"},
        "check_on_launch": "Buscar actualizaciones al iniciar", "check_now": "Buscar actualizaciones",
        "install_update": "Instalar actualización", "region_to_reset": "Región a restablecer",
        "reset_capture": "Restablecer capturas", "hide_footer": "Ocultar la barra inferior", "language": "Idioma",
        "download_pack": "Descargar el pack de imágenes HD", "extract_pack": "Extraer el pack de imágenes HD",
        "value": {"checking": "Buscando…", "up_to_date": "Actualizado", "check_failed": "Error",
                  "no_update": "Ninguna", "downloaded": "Descargado", "pack_size": "75 MB",
                  "not_downloaded": "No descargado"},
    },
    "reset": {
        "all_regions": "Todas las regiones", "drawer_title": "Región a restablecer",
        "all_title": "¿Restablecer todas las capturas?",
        "all_body": "Se borrará el estado de captura de cada Pokémon en todas las regiones. Esta acción no se puede "
                    "deshacer.",
        "region_title": "¿Restablecer {region}?",
        "region_body": "Se borrará el estado de captura de cada Pokémon de {region}. Esta acción no se puede deshacer.",
        "all_button": "Restablecer todo", "region_button": "Restablecer",
        "success_all": "Se han restablecido todas las capturas",
        "success_region": "Se han restablecido las capturas de {region}",
        "failure": "No se pudieron restablecer las capturas",
    },
    "language": {"title": "Idioma", "changed": "Idioma cambiado"},
    "quit": {"title": "¿Salir de pkDex?", "busy": "Hay una descarga en curso y se detendrá."},
    "net": {"offline": "Sin conexión a la red"},
    "update": {
        "toast_available": "pkDex {version} está disponible: instálalo desde Ajustes",
        "latest": "Usas la última versión ({version})", "check_failed": "No se pudo buscar actualizaciones",
        "available_title": "pkDex {version} está disponible",
        "available_body": "Usas la {current}. ¿Descargar e instalar la {version} ahora? pkDex se reinicia al terminar.",
        "install": "Instalar", "none": "No hay ninguna actualización que instalar",
        "screen_title": "Actualizador de pkDex", "sub_running": "Instalando la v{version}",
        "sub_done": "Actualización aplicada", "sub_failed": "La actualización falló",
        "step_download": "Actualización descargada", "step_unpack": "Actualización descomprimida", "step_verify": "Nueva versión verificada",
        "step_backup": "Copia de la versión actual", "step_install": "Actualización instalada",
        "step_cleanup": "Copia eliminada",
        "success": "Actualización aplicada con éxito", "restarting": "Reinicio en {seconds} s",
        "failure": "No se pudo aplicar la actualización", "intact": "se ha conservado la versión actual",
        "keep_on": "Mantén la consola encendida hasta que termine",
    },
    "pack": {
        "exists_title": "Pack de imágenes ya descargado", "exists_body": "¿Quieres descargarlo de nuevo?",
        "redownload": "Volver a descargar", "downloading": "Descargando el pack de imágenes HD",
        "downloaded_title": "Descarga completada",
        "downloaded_body": "Las imágenes en alta resolución están listas para extraerse. ¿Extraerlas ahora?",
        "extract": "Extraer", "cancelled": "Descarga cancelada", "download_failed": "La descarga falló: {error}",
        "missing": "Descarga primero el pack de imágenes", "extract_title": "¿Extraer el pack de imágenes?",
        "extract_body": "Esto puede tardar un poco.", "extracting": "Extrayendo el pack de imágenes HD",
        "extract_failed": "La extracción falló", "extracted_title": "Extracción completada",
        "extracted_body": "Las imágenes en alta resolución ya están disponibles. ¿Quieres conservar el archivo zip "
                          "descargado?",
        "keep": "Conservar", "delete": "Eliminar", "zip_kept": "Se ha conservado el archivo zip",
        "zip_deleted": "Se ha eliminado el archivo zip", "zip_delete_failed": "No se pudo eliminar el archivo zip",
    },
    "about": {
        "eyebrow": "Acerca de", "title": "pkDex", "version": "Versión {version} · por {author}",
        "description": "pkDex es una app sencilla para ver datos de Pokémon sin conexión, para los juegos de Nintendo "
                       "Switch.",
        "source_label": "Código", "source": "github.com/Insektaure/pkDex",
        "license_label": "Licencia", "license": "GNU General Public License v2.0",
        "data_label": "Datos", "data": "serebii.net · pokemondb.net · bulbapedia.bulbagarden.net",
        "logo_label": "Logo", "logo": "Generado con InvokeAI",
        "disclaimer": "pkDex no está afiliado, respaldado ni relacionado con Nintendo, Game Freak ni The Pokémon "
                      "Company. Pokémon y los nombres de los personajes Pokémon son marcas de Nintendo. Se ofrece tal "
                      "cual, solo con fines educativos e informativos.",
    },
    "changelog": {"eyebrow": "Novedades", "title": "Registro de cambios", "missing": "No se encontró el registro de cambios."},
}

IT = {
    "app": {"tagline": "Enciclopedia Pokémon"},
    "sidebar": {"regions": "Regioni", "dlc": "DLC"},
    "nav": {"settings": "Impostazioni", "about": "Info", "changelog": "Novità"},
    "hints": {
        "quit": "Esci", "open": "Apri", "page": "Pagina", "apply": "Applica", "done": "Fatto", "exit": "Esci",
        "select": "Scegli", "status": "Stato", "bulk": "Massa", "multi": "Selezione multipla", "back": "Indietro",
        "previous": "Precedente", "next": "Successivo", "regular_view": "Vista normale",
        "shiny_view": "Vista cromatica", "toggle": "Cambia", "choose": "Scegli",
    },
    "common": {
        "loading": "Caricamento del Pokédex…", "busy": "Un'altra operazione è in corso", "cancel": "Annulla",
        "later": "Più tardi", "ok": "OK", "on": "Sì", "off": "No", "close": "Chiudi",
        "count_pokemon": "{count} Pokémon",
    },
    "regions": regions({
        "kanto": ("Kanto", "Let's Go, Pikachu! e Let's Go, Eevee!", "Let's Go, Pikachu! e Eevee!", "LGPE"),
        "kanto_frlg": ("Kanto", "Rosso Fuoco e Verde Foglia", "Rosso Fuoco e Verde Foglia", "RFVF"),
        "sinnoh": ("Sinnoh", "Diamante Lucente e Perla Splendente", "Diamante Lucente e Perla Splendente", "DLPS"),
        "sinnoh_arceus": ("Sinnoh", "Leggende Pokémon: Arceus", "Leggende: Arceus", "Leggende Arceus"),
        "galar": ("Galar", "Spada e Scudo", "Spada e Scudo", "SpSc"),
        "isle_armor": ("Isola dell'Armatura", "Spada e Scudo · L'isola solitaria dell'armatura", "L'isola solitaria dell'armatura",
                       "SpSc DLC 1"),
        "crown_tundra": ("Landa Corona", "Spada e Scudo · Le terre innevate della corona", "Le terre innevate della corona",
                         "SpSc DLC 2"),
        "paldea": ("Paldea", "Scarlatto e Violetto", "Scarlatto e Violetto", "ScVi"),
        "kitakami": ("Nordivia", "Scarlatto e Violetto · La maschera turchese", "La maschera turchese",
                     "ScVi DLC 1"),
        "blueberry_academy": ("Istituto Mirtillo", "Scarlatto e Violetto · Il disco indaco",
                              "Il disco indaco", "ScVi DLC 2"),
        "kalos_lza": ("Kalos", "Leggende Pokémon: Z-A", "Leggende: Z-A", "Leggende Z-A"),
        "hyperspace_lumiose": ("Luminopoli dimensionale", "Leggende: Z-A · Megadimensione", "Megadimensione",
                               "LZA DLC"),
    }),
    "dex": {
        "eyebrow": "Pokédex regionale", "caught": "catturati", "page": "Pagina {page} di {pages}",
        "multi": "Selezione multipla · {count} scelti", "empty": "Nessun Pokémon in questa regione",
    },
    "states": {"regular": "Normale", "shiny": "Cromatico", "alpha": "Alfa", "shiny_alpha": "Alfa Cromatico"},
    "bulk": {
        "title": "Azioni di massa", "subtitle": "{region} · {count} Pokémon", "mark": "Segna tutti",
        "clear": "Cancella tutti",
        "mark_regular": "Segna tutti come Catturati", "mark_shiny": "Segna tutti come Cromatici",
        "mark_alpha": "Segna tutti come Alfa", "mark_shiny_alpha": "Segna tutti come Alfa Cromatici",
        "clear_regular": "Cancella tutti i Catturati", "clear_shiny": "Cancella tutti i Cromatici",
        "clear_alpha": "Cancella tutti gli Alfa", "clear_shiny_alpha": "Cancella tutti gli Alfa Cromatici",
        "confirm_title": "Modificare {count} Pokémon?",
        "confirm_mark_body": "Tutti i Pokémon di {region} saranno segnati come {state}. Questa azione non può essere "
                             "annullata.",
        "confirm_clear_body": "{state} sarà cancellato per tutti i Pokémon di {region}. Questa azione non può essere "
                              "annullata.",
        "apply": "Applica", "done": "{count} Pokémon modificati",
    },
    "multi": {
        "on": "Selezione multipla: A sceglie, Y applica uno stato", "none": "Scegli prima dei Pokémon con A",
        "title": "Pokémon scelti", "subtitle": "{count} scelti · {region}",
    },
    "capture": {"title": "Stato di cattura"},
    "detail": {
        "previous": "Precedente", "next": "Successivo", "no_sprite": "Nessuna immagine",
        "numbers": "Nazionale #{national} · {region} #{regional}", "national_no": "N° Nazionale",
        "regional_no": "N° Regionale", "shiny": "Cromatico", "available": "Disponibile", "locked": "Bloccato",
        "version": "Esclusiva versione", "none": "Nessuna", "evolution": "Evoluzione", "current": "Attuale",
        "no_evolution": "Non si evolve", "locations": "Luoghi", "unknown_location": "Nessun luogo noto",
        "rows": "Righe {first}–{last} di {total}",
    },
    "settings": {
        "eyebrow": "Preferenze", "title": "Impostazioni",
        "group": {"updates": "Aggiornamenti", "pokemon_data": "Dati Pokémon", "user_interface": "Interfaccia",
                  "language": "Lingua", "resources": "Risorse"},
        "check_on_launch": "Cerca aggiornamenti all'avvio", "check_now": "Cerca aggiornamenti",
        "install_update": "Installa aggiornamento", "region_to_reset": "Regione da azzerare",
        "reset_capture": "Azzera le catture", "hide_footer": "Nascondi la barra inferiore", "language": "Lingua",
        "download_pack": "Scarica il pacchetto immagini HD", "extract_pack": "Estrai il pacchetto immagini HD",
        "value": {"checking": "Ricerca…", "up_to_date": "Aggiornato", "check_failed": "Errore",
                  "no_update": "Nessuno", "downloaded": "Scaricato", "pack_size": "75 MB",
                  "not_downloaded": "Non scaricato"},
    },
    "reset": {
        "all_regions": "Tutte le regioni", "drawer_title": "Regione da azzerare",
        "all_title": "Azzerare tutte le catture?",
        "all_body": "Lo stato di cattura di ogni Pokémon sarà cancellato per tutte le regioni. Questa azione non può "
                    "essere annullata.",
        "region_title": "Azzerare {region}?",
        "region_body": "Lo stato di cattura di ogni Pokémon sarà cancellato per {region}. Questa azione non può essere "
                       "annullata.",
        "all_button": "Azzera tutto", "region_button": "Azzera",
        "success_all": "Tutte le catture sono state azzerate",
        "success_region": "Le catture di {region} sono state azzerate",
        "failure": "Impossibile azzerare le catture",
    },
    "language": {"title": "Lingua", "changed": "Lingua cambiata"},
    "quit": {"title": "Uscire da pkDex?", "busy": "Un download è in corso e verrà interrotto."},
    "net": {"offline": "Nessuna connessione di rete"},
    "update": {
        "toast_available": "pkDex {version} è disponibile: installalo dalle Impostazioni",
        "latest": "Stai usando l'ultima versione ({version})",
        "check_failed": "Impossibile cercare aggiornamenti",
        "available_title": "pkDex {version} è disponibile",
        "available_body": "Stai usando la {current}. Scaricare e installare la {version} ora? pkDex si riavvia al "
                          "termine.",
        "install": "Installa", "none": "Nessun aggiornamento da installare",
        "screen_title": "Aggiornamento di pkDex", "sub_running": "Installazione della v{version}",
        "sub_done": "Aggiornamento applicato", "sub_failed": "Aggiornamento non riuscito",
        "step_download": "Aggiornamento scaricato", "step_unpack": "Aggiornamento estratto", "step_verify": "Nuova versione verificata",
        "step_backup": "Versione attuale salvata", "step_install": "Aggiornamento installato",
        "step_cleanup": "Copia di backup rimossa",
        "success": "Aggiornamento applicato con successo", "restarting": "Riavvio tra {seconds} s",
        "failure": "Impossibile applicare l'aggiornamento", "intact": "la versione attuale è stata mantenuta",
        "keep_on": "Tieni la console accesa fino al termine",
    },
    "pack": {
        "exists_title": "Pacchetto immagini già scaricato", "exists_body": "Vuoi scaricarlo di nuovo?",
        "redownload": "Riscarica", "downloading": "Download del pacchetto immagini HD",
        "downloaded_title": "Download completato",
        "downloaded_body": "Le immagini ad alta risoluzione sono pronte per l'estrazione. Estrarle ora?",
        "extract": "Estrai", "cancelled": "Download annullato", "download_failed": "Download non riuscito: {error}",
        "missing": "Scarica prima il pacchetto immagini", "extract_title": "Estrarre il pacchetto immagini?",
        "extract_body": "Potrebbe richiedere un po' di tempo.", "extracting": "Estrazione del pacchetto immagini HD",
        "extract_failed": "Estrazione non riuscita", "extracted_title": "Estrazione completata",
        "extracted_body": "Le immagini ad alta risoluzione sono ora disponibili. Vuoi conservare il file zip scaricato?",
        "keep": "Conserva", "delete": "Elimina", "zip_kept": "Il file zip è stato conservato",
        "zip_deleted": "Il file zip è stato eliminato", "zip_delete_failed": "Impossibile eliminare il file zip",
    },
    "about": {
        "eyebrow": "Info", "title": "pkDex", "version": "Versione {version} · di {author}",
        "description": "pkDex è un'app semplice per visualizzare i dati dei Pokémon offline, per i giochi Nintendo "
                       "Switch.",
        "source_label": "Sorgente", "source": "github.com/Insektaure/pkDex",
        "license_label": "Licenza", "license": "GNU General Public License v2.0",
        "data_label": "Dati", "data": "serebii.net · pokemondb.net · bulbapedia.bulbagarden.net",
        "logo_label": "Logo", "logo": "Generato con InvokeAI",
        "disclaimer": "pkDex non è affiliato, approvato o collegato a Nintendo, Game Freak o The Pokémon Company. "
                      "Pokémon e i nomi dei personaggi Pokémon sono marchi di Nintendo. Fornito così com'è, solo a "
                      "scopo educativo e informativo.",
    },
    "changelog": {"eyebrow": "Novità", "title": "Registro modifiche", "missing": "File delle modifiche non trovato."},
}

JA = {
    "app": {"tagline": "ポケモン図鑑"},
    "sidebar": {"regions": "地方", "dlc": "DLC"},
    "nav": {"settings": "設定", "about": "情報", "changelog": "更新履歴"},
    "hints": {
        "quit": "終了", "open": "開く", "page": "ページ", "apply": "適用", "done": "完了", "exit": "やめる",
        "select": "選ぶ", "status": "状況", "bulk": "一括", "multi": "複数選択", "back": "戻る", "previous": "前へ",
        "next": "次へ", "regular_view": "通常の姿", "shiny_view": "色違いの姿", "toggle": "切り替え", "choose": "決定",
    },
    "common": {
        "loading": "図鑑を読み込み中…", "busy": "ほかの処理を実行中です", "cancel": "キャンセル", "later": "あとで",
        "ok": "OK", "on": "オン", "off": "オフ", "close": "閉じる", "count_pokemon": "{count}匹",
    },
    "regions": regions({
        "kanto": ("カントー", "Let's Go! ピカチュウ・Let's Go! イーブイ", "Let's Go! ピカチュウ・イーブイ", "LGPE"),
        "kanto_frlg": ("カントー", "ファイアレッド・リーフグリーン", "ファイアレッド・リーフグリーン", "FRLG"),
        "sinnoh": ("シンオウ", "ブリリアントダイヤモンド・シャイニングパール", "ブリリアントダイヤモンド・シャイニングパール", "BDSP"),
        "sinnoh_arceus": ("シンオウ", "Pokémon LEGENDS アルセウス", "LEGENDS アルセウス", "LEGENDS アルセウス"),
        "galar": ("ガラル", "ソード・シールド", "ソード・シールド", "剣盾"),
        "isle_armor": ("ヨロイじま", "ソード・シールド・鎧の孤島", "鎧の孤島", "剣盾 DLC1"),
        "crown_tundra": ("カンムリせつげん", "ソード・シールド・冠の雪原", "冠の雪原", "剣盾 DLC2"),
        "paldea": ("パルデア", "スカーレット・バイオレット", "スカーレット・バイオレット", "SV"),
        "kitakami": ("キタカミの里", "スカーレット・バイオレット・碧の仮面", "碧の仮面", "SV DLC1"),
        "blueberry_academy": ("ブルーベリー学園", "スカーレット・バイオレット・藍の円盤", "藍の円盤",
                              "SV DLC2"),
        "kalos_lza": ("カロス", "Pokémon LEGENDS Z-A", "LEGENDS Z-A", "LEGENDS Z-A"),
        "hyperspace_lumiose": ("異次元ミアレ", "LEGENDS Z-A・M次元ラッシュ", "M次元ラッシュ", "LZA DLC"),
    }),
    "dex": {
        "eyebrow": "地方図鑑", "caught": "捕獲済み", "page": "{page} / {pages} ページ",
        "multi": "複数選択・{count}匹選択中", "empty": "この地方にはポケモンがいません",
    },
    "states": {"regular": "ノーマル", "shiny": "色違い", "alpha": "オヤブン", "shiny_alpha": "色違いオヤブン"},
    "bulk": {
        "title": "一括操作", "subtitle": "{region}・{count}匹", "mark": "すべて登録", "clear": "すべてクリア",
        "mark_regular": "すべてを捕獲済みにする", "mark_shiny": "すべてを色違い捕獲済みにする",
        "mark_alpha": "すべてをオヤブン捕獲済みにする", "mark_shiny_alpha": "すべてを色違いオヤブン捕獲済みにする",
        "clear_regular": "すべての捕獲済みをクリア", "clear_shiny": "すべての色違いをクリア",
        "clear_alpha": "すべてのオヤブンをクリア", "clear_shiny_alpha": "すべての色違いオヤブンをクリア",
        "confirm_title": "{count}匹のポケモンを変更しますか？",
        "confirm_mark_body": "{region}のすべてのポケモンが「{state}」として登録されます。この操作は元に戻せません。",
        "confirm_clear_body": "{region}のすべてのポケモンの「{state}」がクリアされます。この操作は元に戻せません。",
        "apply": "適用", "done": "{count}匹を変更しました",
    },
    "multi": {
        "on": "複数選択：Aで選択、Yで状況を適用", "none": "まずAでポケモンを選んでください",
        "title": "選択したポケモン", "subtitle": "{count}匹選択中・{region}",
    },
    "capture": {"title": "捕獲状況"},
    "detail": {
        "previous": "前へ", "next": "次へ", "no_sprite": "画像なし",
        "numbers": "全国 #{national}・{region} #{regional}", "national_no": "全国図鑑No.",
        "regional_no": "地方図鑑No.", "shiny": "色違い", "available": "入手可能", "locked": "入手不可",
        "version": "バージョン限定", "none": "なし", "evolution": "進化", "current": "現在",
        "no_evolution": "進化しない", "locations": "出現場所", "unknown_location": "出現場所は不明",
        "rows": "{first}–{last}行目 / 全{total}行",
    },
    "settings": {
        "eyebrow": "設定", "title": "設定",
        "group": {"updates": "アップデート", "pokemon_data": "ポケモンデータ", "user_interface": "表示",
                  "language": "言語", "resources": "リソース"},
        "check_on_launch": "起動時にアップデートを確認", "check_now": "アップデートを確認",
        "install_update": "アップデートをインストール", "region_to_reset": "リセットする地方",
        "reset_capture": "捕獲状況をリセット", "hide_footer": "下部バーを隠す", "language": "言語",
        "download_pack": "高解像度画像パックをダウンロード", "extract_pack": "高解像度画像パックを展開",
        "value": {"checking": "確認中…", "up_to_date": "最新", "check_failed": "失敗", "no_update": "なし",
                  "downloaded": "ダウンロード済み", "pack_size": "75 MB", "not_downloaded": "未ダウンロード"},
    },
    "reset": {
        "all_regions": "すべての地方", "drawer_title": "リセットする地方",
        "all_title": "すべての捕獲状況をリセットしますか？",
        "all_body": "すべての地方で、すべてのポケモンの捕獲状況が消去されます。この操作は元に戻せません。",
        "region_title": "{region}をリセットしますか？",
        "region_body": "{region}のすべてのポケモンの捕獲状況が消去されます。この操作は元に戻せません。",
        "all_button": "すべてリセット", "region_button": "リセット",
        "success_all": "すべての捕獲状況をリセットしました",
        "success_region": "{region}の捕獲状況をリセットしました",
        "failure": "捕獲状況をリセットできませんでした",
    },
    "language": {"title": "言語", "changed": "言語を変更しました"},
    "quit": {"title": "pkDexを終了しますか？", "busy": "ダウンロード中です。終了すると中断されます。"},
    "net": {"offline": "ネットワークに接続されていません"},
    "update": {
        "toast_available": "pkDex {version} が利用できます。設定からインストールできます",
        "latest": "最新バージョン（{version}）を使用しています", "check_failed": "アップデートを確認できませんでした",
        "available_title": "pkDex {version} が利用できます",
        "available_body": "現在のバージョンは {current} です。{version} をダウンロードしてインストールしますか？完了後に pkDex が再起動します。",
        "install": "インストール", "none": "インストールできるアップデートはありません",
        "screen_title": "pkDex アップデーター", "sub_running": "v{version} をインストール中",
        "sub_done": "アップデート完了", "sub_failed": "アップデート失敗",
        "step_download": "アップデートをダウンロード", "step_unpack": "アップデートを展開", "step_verify": "新しいバージョンを確認",
        "step_backup": "現在のバージョンをバックアップ", "step_install": "アップデートをインストール",
        "step_cleanup": "バックアップを削除",
        "success": "アップデートが完了しました", "restarting": "{seconds} 秒後に再起動",
        "failure": "アップデートを適用できませんでした", "intact": "現在のバージョンはそのままです",
        "keep_on": "完了するまで本体の電源を切らないでください",
    },
    "pack": {
        "exists_title": "画像パックはダウンロード済みです", "exists_body": "もう一度ダウンロードしますか？",
        "redownload": "再ダウンロード", "downloading": "高解像度画像パックをダウンロード中",
        "downloaded_title": "ダウンロード完了", "downloaded_body": "高解像度画像を展開できます。今すぐ展開しますか？",
        "extract": "展開", "cancelled": "ダウンロードをキャンセルしました", "download_failed": "ダウンロード失敗：{error}",
        "missing": "先に画像パックをダウンロードしてください", "extract_title": "画像パックを展開しますか？",
        "extract_body": "しばらく時間がかかる場合があります。", "extracting": "高解像度画像パックを展開中",
        "extract_failed": "展開に失敗しました", "extracted_title": "展開完了",
        "extracted_body": "高解像度画像が使えるようになりました。ダウンロードしたzipファイルを残しますか？",
        "keep": "残す", "delete": "削除", "zip_kept": "zipファイルを残しました", "zip_deleted": "zipファイルを削除しました",
        "zip_delete_failed": "zipファイルを削除できませんでした",
    },
    "about": {
        "eyebrow": "情報", "title": "pkDex", "version": "バージョン {version}・{author}",
        "description": "pkDexは、ニンテンドースイッチ用ポケモンゲームのデータをオフラインで閲覧できるシンプルなアプリです。",
        "source_label": "ソース", "source": "github.com/Insektaure/pkDex",
        "license_label": "ライセンス", "license": "GNU General Public License v2.0",
        "data_label": "データ", "data": "serebii.net・pokemondb.net・bulbapedia.bulbagarden.net",
        "logo_label": "ロゴ", "logo": "InvokeAIで生成",
        "disclaimer": "pkDexは任天堂、ゲームフリーク、株式会社ポケモンとは一切関係がなく、承認も受けていません。"
                      "ポケモンおよびポケモンのキャラクター名は任天堂の商標です。教育・情報提供のみを目的として、現状のまま提供されます。",
    },
    "changelog": {"eyebrow": "新着情報", "title": "更新履歴", "missing": "更新履歴ファイルが見つかりません。"},
}

LOCALES = {"en-US": EN, "fr-FR": FR, "de-DE": DE, "es-ES": ES, "it-IT": IT, "ja-JP": JA}


def keys(d, prefix=""):
    out = set()
    for k, v in d.items():
        p = prefix + "/" + k
        out |= keys(v, p) if isinstance(v, dict) else {p}
    return out


ref = keys(EN)
ok = True
for code, table in LOCALES.items():
    k = keys(table)
    if k != ref:
        ok = False
        print(code, "missing:", sorted(ref - k), "extra:", sorted(k - ref))
if not ok:
    sys.exit(1)

for code, table in LOCALES.items():
    d = os.path.join(ROOT, "resources", "i18n", code)
    os.makedirs(d, exist_ok=True)
    with open(os.path.join(d, "pkdex.json"), "w", encoding="utf-8") as f:
        json.dump(table, f, ensure_ascii=False, indent=2)
        f.write("\n")
print("wrote", len(LOCALES), "locales,", len(ref), "keys each")
