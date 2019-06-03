# Design choices - COW & Dedup

# Déduplication de blocs

La déduplication des blocs peut être faite inline ou offline. Pour ce projet, tout le processus de déduplication se fait offline lors du démontage des volumes. La solution naïve pour implémenter ceci est de faire une comparaison de tous les blocs 1 à 1 pour déterminer l'égalité. On a choisi d'implémenter directement une solution plus performante basée sur le calcul de checksum des blocs.

L'idée est d'éviter cette comparaison en temps quadratique, en étant capables d'indexer une représentation de ce bloc de taille inférieur(256bits de hash vs 4KB de bloc). Certaines problématiques se posent par rapport au calcul de cette représentation et à son stockage. On part du principe que la déduplication va être un processus très coûteux en mémoire et en CPU lorsque la déduplication est faite offline.

## Choix du checksum

### Performance
Il existe différents algorithmes pour calculer un checksum unique à partir d'une série de données, dans notre cas un bloc. Certaines de ces fonctions de hash offrent une performance excellente, au détriment d'un manque de resistance aux attaques par pré-image (murmur3), d'autres offrent des garanties sur le nombre de collisions et sur la sécurité, au détriment de la vitesse de hashage(sha256).

### Cryptographique ou pas cryptographique ?
On pourrait penser dans un premier temps que dans le cas d'une déduplication cette résistance à la pré-image est inutile, cependant, dans un contexte multi-utilisateur, certaines contraintes s'appliquent. Un utilisateur malveillant pourrait être capable de générer de collisions volontairement pour avoir accès au bloc de données d'un autre utilisateur par exemple. Pour ceci, on choisi d'utiliser une fonction de hachage cryptographiquement sécurisée. En contrepartie, on a un temps de calcul des hashs plus long, qui affectera le temps de démontage.

On a choisi SHA256, un algorithme avec [une probabilité de collision très faible (10^-77)](https://blogs.oracle.com/bonwick/zfs-deduplication-v2). Ceci nous permet d'éviter d'avoir à gérer les collisions dans le hash des blocs, en nous garantissant qu'un hash identique correspond toujours à des blocs identiques. Cet algorithme est déjà implémenté dans le kernel.

## Stockage et indexation des hash

### Stockage des hash

Il est connu que la structure de données qui permet l'indexation avec les accès les plus rapides est la table de hashs. La taille de cette table de hachage dépend cependant de l'algorithme de hachage et de possibles valeurs de sortie de celui-ci. La clé d'indexation serait dans notre cas le hash d'un bloc, qui nous permettrait de retrouver le bloc, s'il existe, qui contiendrait les mêmes données. Le problème de cette structure de données est sa taille. La quantitée de valeurs possibles de SHA256 est 2^256, stockées dans 32 bytes, cela nous fairait une table de 3.5x10^69 GB en mémoire.

On chosit donc une structure de données avec un recherche plus modeste, un arbre binaire. L'arbre binaire offrant la meilleure recherche dans le pire cas est un AVL, mais on préfère prendre avantage des structures de données déjà implémentées dans le noyau. Pour les arbres binaires, celui avec la meilleure recherche est donc l'arbre rouge noir, avec un pire cas pour la recherche légèrement plus grand que celui des AVL.

La façon dont on procède pour vérifier si un bloc existe déjà est la suivante:

1. On obtient la liste des inodes utilisées
2. On parcours la liste des blocs de cette inode
3. On hash chaque block et on le cherche dans notre arbre
    * s'il est déjà dedans, on fait pointer l'inode (index block) vers le bloc qui est déjà indéxé,
    * s'il n'est pas dedans on l'insére et on passe au block suivant

À la fin on a parcouru tous les blocs des inodes utilisés et on les a fait pointer vers la première apparition de ce bloc, idéalement en O(nlogn), n pour le parcours de tous les blocs utilisés, logn pour la recherche et insertion dans l'arbre.

### Une possible alternative

Une option qui nous permettrait de réduire l'usage mémoire pour une table de hashs serait une table de hash partielle. Au lieu d'utiliser des hashs de taille 32 octets, on utiliserait des données plus petites, mais sans garanties de non-collision, stockées en mémoire. Si jamais une collision était trouvée, on passerait une deuxième vérification sur un hash complet SHA256 en disque. Ceci implique des pertes dans la performance à évaluer.

### Inline ou offline

Même si la déduplication est faite offline (à cause des synchronisations du kernel sur les inodes et la gestion de la cache), le calcul et stockage des hashs pourrait toujours être fait lors de l'écriture d'un bloc. Au démontage il suffirait de parcourir la liste des hashs déjà calculés et retrouver les blocs équivalents. Le problème de cette approche est la mise à jour du hash d'un bloc lors de la modification de celui-ci. Ça éxigerait d'indexer d'un côté, le bloc correspondant à un hash (clé: hash, valeur: block) et d'un autre le hash correspondant à un block (clé: block, valeur: hash). Ceci implique deux structures en mémoire ou la modification de notre arbre pour permettre les deux recherches.

## Copy On Write

Encore une fois COW est un opération avec un cout en mémoire très élevée.

## Block modifiable ou block à copier?

Après avoir réalisé une déduplication on se retrouve avec le problème de blocs partagés. On doit être capables de savoir quels blocs sont partagés et quels blocs on peut modifier de façon sûre. Pour les blocs partagés il faudra réaliser un copie avant de pouvoir les modifier.

L'idée initiale est d'avoir un compteur de références pour chaque bloc. Vu que cette information serait calculée au démontage (lors de la déduplication) car c'est le seul moment ou on peut connaître l'info, il faudrait la stocker en disque avant le démontage. Lors du montage on pourrait la charger en mémoire pour vérifier avant chaque écriture sur un bloc si le compteur de références sur celui-ci est 0, et donc on peut écrire dessus directement, ou >1, et il faut allouer un nouveau bloc pour écrire notre information.

Le début de cette implémentation a été faite, cependant on arrive pas à avoir une version fonctionnelle, à cause d'un problème du stockage des compteurs de référence en disque. 

## Alternative

Une alternative pour éviter le stockage en disque des compteurs de références est de les calculer au montage et de les avoir en mémoire. Ceci implique une opération très lourde et on a choisi de ne pas implementer cette solution.
