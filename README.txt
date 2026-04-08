# MRTD_Automatisation_GPIB

Banc de test automatique pour mesurer la MRTD d’une caméra thermique
en pilotant un corps noir via GPIB et une UI GTK4 sur Raspberry Pi 4.

## Fonctionnalités

- Contrôle du corps noir via GPIB (GPIB-USB-HS NI, linux-gpib).
- Interface graphique GTK4 (écran tactile 5").
- Acquisition et traitement d’image avec OpenCV (calcul MRTD).
- Mode développement sur PC + exécution sur Raspberry Pi.

## Prérequis

- Raspberry Pi 4 (4 Go RAM), Raspberry Pi OS 64 bits.
- Adaptateur NI GPIB-USB-HS.
- Paquets :
  - `gtk4`, `gtk4-devel` ou équivalent.
  - `linux-gpib`, `linux-gpib-dev`.
  - `cmake`, `gcc`, `make`.
  - OpenCV (version X.Y).

## Installation de base

Rasberry OS LITE Bookworm ARM64.

-Activer SSH dans raspi-config
-Mettre à jour l'os & nettoyer les paquets inutiles

sudo apt update
sudo apt full-upgrade -y
sudo apt autoremove -y
sudo reboot

-Pour inclure VScode dans la toolchain, simplement installé le framework "Remote explorer" sur l'IDE.
entrée l'ip du rasberry (sudo hostname -I) et le serveur VScode s'installera si la connection est établie.

## Installation des dépendance

Depuis la fenêtre de Remote SSH dans vscode l'on peut aller dans l'onglet extensions et installer sur le rasberry:

- C/C++
- C/C++ Extension pack
- C/C++ DevTools
- Makefile Tools
- Cmake Tools

## Ajout du driver GPIB au kernel

-Partie Kernel-
sudo apt install build-essential raspberrypi-kernel-headers dkms subversion
git clone https://github.com/coolshou/linux-gpib.git
cd linux-gpib-kernel
make
sudo make install

-Partie user-
cd ~/driver/linux-gpib/linux-gpib-user
make
sudo make install
sudo ldconfig

sudo nano /usr/local/etc/gpib.conf

modifier les lignes: 
  board_type = "ni_pci" --> board_type = "ni_usb_b"
  pad = 0 --> pad = 1

## Installation serveur graphique X11 & gtk3

sudo apt update
sudo apt install --no-install-recommends xserver-xorg x11-xserver-utils xinit
sudo apt install libgtk-3-dev
sudo apt install openbox


