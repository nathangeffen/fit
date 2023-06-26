import unittest
import grid

DOMAINS = [[-10.0, 5.0], [-20.0, 45.0], [-15.0, 100.0], [-95.0, 80.]]


class TestGridOptimization(unittest.TestCase):

    def test_random_sphere(self):
        o = grid.Optimization('random', grid.sphere, DOMAINS, kwargs={
            'iterations': 100
            })
        result = o.optimize()
        self.assertEqual(len(result['vector']), len(DOMAINS))
        self.assertTrue(result['func'] > 0.0)
        self.assertAlmostEqual(grid.sphere(result['vector']), result['func'])

    def test_grid_sphere(self):
        o = grid.Optimization('grid', grid.sphere, DOMAINS, kwargs={
            'generations': 4,
            'divisions': [5, 4, 10, 8],
            'passes': 4
            })
        result = o.optimize()
        self.assertEqual(len(result['vector']), len(DOMAINS))
        self.assertTrue(result['func'] < 0.1)
        self.assertAlmostEqual(grid.sphere(result['vector']), result['func'])

    def test_grid_rastrigin(self):
        o = grid.Optimization('grid', grid.rastrigin, DOMAINS, kwargs={
            'generations': 4,
            'divisions': [5, 4, 10, 8],
            'passes': 4
            })
        result = o.optimize()
        self.assertEqual(len(result['vector']), len(DOMAINS))
        self.assertTrue(result['func'] < 11.0)
        self.assertAlmostEqual(grid.rastrigin(result['vector']), result['func'])

    def test_grid_external(self):
        o = grid.Optimization('grid', grid.external, DOMAINS, command='./sphere',
                kwargs={
                    'generations': 4,
                    'divisions': [5, 4, 10, 8],
                    'passes': 4
                    })
        result = o.optimize()
        self.assertEqual(len(result['vector']), len(DOMAINS))
        self.assertTrue(result['func'] < 0.1)
        self.assertAlmostEqual(grid.sphere(result['vector']), result['func'], 4)

    def test_grid_stress(self):
        domains = grid.make_domains(100, [-105.0], [50.0])
        divisions = [20] * 100
        o = grid.Optimization('grid', grid.rastrigin, domains, max_jobs=4,
                kwargs={
                    'generations': 4,
                    'divisions': divisions,
                    'passes': 4
                    })
        result = o.optimize()
        self.assertEqual(len(result['vector']), len(domains))
        self.assertTrue(result['func'] < 0.1)
        self.assertAlmostEqual(grid.rastrigin(result['vector']), result['func'])


if __name__ == '__main__':
    unittest.main()
