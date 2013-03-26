from pyxrootd import client

class CopyProcess(object):
  """TODO: write me"""
  
  def __init__(self):
    self.__process = client.CopyProcess()
    
  def add_job(self, job):
    """Add a job"""
    self.__process.add_job(job)
    
  def prepare(self):
    """Prepare the jobs"""
    self.__process.prepare()
    
  def run(self):
    """Run the jobs"""
    self.__process.run()