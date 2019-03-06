require './app'
require './middlewares/backend'

use Tape::Backend

run Tape::App
