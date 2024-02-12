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

void compile_features()
{
}
