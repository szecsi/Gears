
class SequenceError(Exception) : 
    def __init__(self, desc, tb):
        super().__init__(desc)
        self.tb = tb
