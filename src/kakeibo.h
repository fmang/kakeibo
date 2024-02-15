#pragma once

#include <opencv2/core.hpp>

#include <array>
#include <string>
#include <string_view>
#include <vector>

/**
 * Mode d’opération :
 * - c : Découpe les reçus présents sur une photo.
 * - x : Extrait les fragments intéressantes d’un reçu.
 */
extern char mode;

/**
 * Si activé via --debug, affiche visuellement les données traitées.
 */
extern bool debug;

/**
 * Reçoit un contour contenant 4 points et réordonne les points dans le sens
 * horaire, en commençant par le coin haut-gauche.
 */
struct quad {
	quad(const std::vector<cv::Point>& corners);
	std::array<cv::Point, 4> corners;

	int height() const;
	int width() const;
	void shrink(int border);
};

/**
 * Reçoit une image et renvoie la liste des reçus extraits. Chaque reçu est
 * recadré pour rentrer dans un rectangle droit de 600 px de large pour une
 * résolution d’environ 10 px / mm.
 */
std::vector<cv::Mat> cut_receipts(cv::Mat photo);

/**
 * Enregistre l’image dans un fichier extracted/0123.png. Renvoie le nom du
 * fichier de sortie.
 */
std::string save(cv::Mat image);

/**
 * Ouvre une fenêtre affichant l’image. Au plus une image à la fois est
 * affichée. Attend que l’utilisateur appuie sur une touche pour passer à
 * l’image suivante. Si l’utilisateur appuie sur une autre touche qu’Espace,
 * toutes les demandes d’affichage successives de la même image seront
 * ignorées.
 *
 * Nécessite --debug pour être activée.
 */
void show(const std::string& name, cv::Mat image);

/**
 * Extrait les informations d’un reçu.
 */
void scan_receipt(cv::Mat photo);

/**
 * Extrait dans pleins de petits fichiers chaque lettre contenu dans le reçu.
 * Ces images sont destinées à servir d’échantillons pour le moteur de
 * reconnaissance de glyphes.
 */
void extract_samples(cv::Mat photo);

/**
 * Lit tous les fichiers sous samples/ et construit letters.txt avec les
 * features extraites des images.
 */
void compile_features();
