import csv

csv.register_dialect("tsv", delimiter="\t", lineterminator="\n")
