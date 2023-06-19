import unittest
import grid

DOMAINS = [[-10.0, 5.0], [-20.0, 45.0], [-15.0, 100.0], [-95.0, 80.]]


class TestGridOptimization(unittest.TestCase):

    def test_random_sphere(self):
        o = grid.Optimization('random', 'sphere', DOMAINS, kwargs={
                'iterations': 100
            })
        result = o.optimize()
        self.assertEquals(len(result['vector']), len(DOMAINS))
        self.assertTrue(result['func'] > 0.0)

    def test_grid_sphere(self):
        o = grid.Optimization('grid', 'sphere', DOMAINS, kwargs={
                'generations': 4,
                'divisions': [5, 4, 10, 8],
                'passes': 4
            })
        result = o.optimize()
        self.assertEquals(len(result['vector']), len(DOMAINS))
        self.assertTrue(result['func'] < 0.1)

    def test_grid_rastrigin(self):
        o = grid.Optimization('grid', 'rastrigin', DOMAINS, kwargs={
                'generations': 4,
                'divisions': [5, 4, 10, 8],
                'passes': 4
            })
        result = o.optimize()
        self.assertEquals(len(result['vector']), len(DOMAINS))
        self.assertTrue(result['func'] < 11.0)


if __name__ == '__main__':
    unittest.main()
