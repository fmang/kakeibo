/**
 * Plutôt que de comparer naïvement les pixels des échantillons, essayons d’en
 * extraire les propriétés. Le papier ci-dessous propose de faire des
 * projections sur des cellules, ce qui parait être une bonne idée. Comme les
 * kanjis ont facilement des barres au milieu, prenons un nombre impair de
 * cellules. Pour insister sur les bords, on pourrait aussi faire en sorte que
 * les cellules du bord soient plus petites, et celle du centre plus grande.
 *
 * Concernant le scaling, visons un format standard carré. Ceci dit, pour
 * éviter que les glyphes étroits comme 1 soient trop déformés, interdisons le
 * scaling passée une limite, et complétons avec des bords noirs.
 *
 * Comme les images sont de basse résolution, plutôt que d’appliquer le scaling
 * sur l’image source, appliquons-le sur la grille qui servira à la projection.
 * Sur une image carrée, les cellules feront 1, 2, 3, 2, et 1 neuvième de
 * l’image pour chaque dimension. Les 5 cellules cumulées ne devront cependant
 * pas faire moins de 2/3 de l’autre dimension. Si elles sortent de l’image, on
 * considèrera que la projection vaut 0.
 *
 * Acceptons --debug avec show() pour débugger visuellement l’extraction des
 * features.
 *
 * Référence :
 * - Rapid Feature Extraction for Optical Character Recognition
 *   <https://arxiv.org/pdf/1206.0238.pdf>
 */

#include "kakeibo.h"

/**
 * Indique comment découper une image en 5 cellules selon une dimension de
 * l’image, et la dimension croisée. La 2e dimension sert à éviter de trop
 * étirer l’image.
 *
 * Renvoie 6 coordonnées définissant les bords des 5 cellules.
 */
static std::vector<int> split_axis(int main_size, int cross_size)
{
	int middle = main_size / 2;
	int min_size = cross_size * 2 / 3;
	main_size = std::min(main_size, min_size);

	int center_size = main_size / 3;
	int lateral_size = (main_size - center_size) / 3;

	return {
		middle - main_size / 2,
		middle - center_size - lateral_size,
		middle - center_size,
		middle + center_size,
		middle + center_size + lateral_size,
		middle + main_size / 2 + main_size % 2,
	};
}

static std::vector<cv::Rect> build_cells(int width, int height)
{
	std::vector<cv::Rect> cells;

	std::vector<int> horizontal_rule = split_axis(width, height);
	for (size_t i = 1; i < horizontal_rule.size(); ++i) {
		int cell_x = horizontal_rule[i - 1];
		int cell_width = horizontal_rule[i] - cell_x;
		cells.emplace_back(cell_x, 0, cell_width, height);
	}

	std::vector<int> vertical_rule = split_axis(height, width);
	for (size_t i = 1; i < vertical_rule.size(); ++i) {
		int cell_y = vertical_rule[i - 1];
		int cell_height = vertical_rule[i] - cell_y;
		cells.emplace_back(cell_y, 0, width, cell_height);
	}

	return cells;
}

void compile_features()
{
}
