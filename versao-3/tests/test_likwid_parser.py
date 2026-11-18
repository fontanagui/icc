#!/usr/bin/env python3
"""Testes de formato com entradas de teste, excluídas da campanha experimental."""

from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "benchmark"))
from formato import REGIOES, le_likwid, le_tempos


class TesteCsvLikwid(unittest.TestCase):
    def le(self, grupo, metrica, dado="16.25", chamadas=25, cpu=6):
        blocos = [
            f"STRUCT,Info,3,,,\nCPU type:,CPU de teste,,,\n"
            f"TABLE,Region {r},Group 1 Raw,{grupo},1,\n"
            f"Region Info,HWThread {cpu},\nRDTSC Runtime [s],0.000123,\ncall count,{chamadas},\n"
            f"Event,Counter,HWThread {cpu},\nINSTR_RETIRED,FIXC0,123,\n"
            f"TABLE,Region {r},Group 1 Metric,{grupo},2,\n"
            f"Metric,HWThread {cpu},\nRuntime (RDTSC) [s],0.000123,\n{metrica},{dado},\n"
            f"TABLE,Region {r},Group 1 Metric STAT,{grupo},1,\n"
            f"Metric,Sum,Min,Max,Avg\n{metrica} STAT,900,900,900,900\n"
            for r in REGIOES
        ]
        with tempfile.TemporaryDirectory() as pasta:
            caminho = Path(pasta) / "perfctr.csv"
            caminho.write_text("".join(blocos), encoding="utf-8")
            return le_likwid(caminho, grupo, 6)

    def test_metricas(self):
        for grupo, metrica in (("MEM", "Memory bandwidth [MBytes/s]"),
                               ("L3", "L3 bandwidth [MBytes/s]"),
                               ("L2CACHE", "L2 miss ratio"),
                               ("CACHE", "data cache miss ratio"),
                               ("FLOPS_AVX", "Packed DP [MFLOP/s]"),
                               ("FLOPS_DP", "DP [MFLOP/s]")):
            with self.subTest(grupo=grupo):
                dados = self.le(grupo, metrica, dado="0")
                self.assertEqual(tuple(dados), REGIOES)
                self.assertEqual(dados["Jacobiana"], (metrica, 0.0, 25, 0.000123))

    def test_rejeita_indefinido(self):
        with self.assertRaisesRegex(ValueError, "indefinido"):
            self.le("FLOPS_DP", "DP [MFLOP/s]", dado="-")

    def test_rejeita_outra_cpu(self):
        with self.assertRaisesRegex(ValueError, "HWThread 6 ausente"):
            self.le("L2CACHE", "L2 miss ratio", cpu=4)

    def test_rejeita_contagens_diferentes(self):
        with self.assertRaisesRegex(ValueError, "esperadas 25"):
            self.le("L2CACHE", "L2 miss ratio", chamadas=24)

    def test_le_tempos(self):
        self.assertEqual(le_tempos("BROYDEN_STATS,25,10,1,8\n"), dict(zip(REGIOES, (10, 1, 8))))
        with self.assertRaises(ValueError):
            le_tempos("BROYDEN_STATS,24,10,1,8\n")


if __name__ == "__main__":
    unittest.main()
