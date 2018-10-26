# Extract API numbers from file (do now, in separate file to guarantee clean
# namespace (except for private __*__ variables))
from api import *
loc = dict(locals())
apis = {d: loc[d] for d in loc.keys() if not (d.startswith('__') or d.endswith('__'))}
