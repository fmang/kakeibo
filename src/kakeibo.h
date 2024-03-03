#pragma once

#include <opencv2/core.hpp>

#include <array>
#include <string>
#include <string_view>
#include <vector>

/**
 * Si activé via --explain, affiche visuellement les données traitées.
 */
extern bool explain;

/**
 * Ouvre une fenêtre affichant l’image. Au plus une image à la fois est
 * affichée. Attend que l’utilisateur appuie sur une touche pour passer à
 * l’image suivante. Si l’utilisateur appuie sur une autre touche qu’Espace,
 * toutes les demandes d’affichage successives de la même image seront
 * ignorées.
 *
 * Nécessite --explain pour être activée.
 */
void show(const std::string& name, cv::Mat image);

/**
 * Enregistre l’image dans un fichier extracted/0123.png. Renvoie le nom du
 * fichier de sortie.
 */
std::string save(cv::Mat image);

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
 * Renvoie la liste des countours des reçus trouvés.
 */
std::vector<quad> find_receipts(cv::Mat photo);

/**
 * Reçoit une image et un des countours trouvés par find_receipts, puis découpe
 * le reçu en question. L’image est recadrée et sa perspective corrigée pour
 * faire 600 px de large, soit une résolution d’environ 10 px / mm.
 */
cv::Mat cut_receipt(cv::Mat photo, quad contour);

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
 * Lit tous les fichiers sous samples/ et écrit sur stdout un CSV avec features
 * extraites des échantillons.
 */
void compile_features();
