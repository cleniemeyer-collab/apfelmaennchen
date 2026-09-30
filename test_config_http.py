import copy
import json
import urllib.error
import urllib.request


def post(route, data):
    return urllib.request.urlopen(urllib.request.Request('http://127.0.0.1:8080'+route,
        data=json.dumps(data).encode(), headers={'Content-Type': 'application/json'}), timeout=55)


if __name__ == '__main__':
    view = {'path': [[400000,150000,250000]], 'iterations': 137, 'backend': 'perturb'}
    with post('/config/export', {'view': view, 'palette': {'offset': 123, 'direction': -1}}) as response:
        config = json.load(response)
    assert config['view'] == view and isinstance(config['bounds']['left'], str)
    with post('/config/import', config) as response:
        loaded = json.load(response)
    assert loaded == config
    with post('/render', view) as response:
        original = response.read()
    with post('/render', loaded['view']) as response:
        assert response.read() == original
    bad = copy.deepcopy(config)
    bad['bounds']['left'] = '0'
    try:
        post('/config/import', bad)
        raise AssertionError('Invalid bounds accepted')
    except urllib.error.HTTPError as error:
        assert error.code == 400
    print('HTTP config export/import: identical image, exact bounds, invalid file rejected.')
