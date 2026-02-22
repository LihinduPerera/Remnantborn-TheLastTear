const { verifyToken } = require('../config/auth');
const { supabase } = require('../config/database');

const authenticate = async (req, res, next) => {
  try {
    const authHeader = req.headers.authorization;
    
    if (!authHeader || !authHeader.startsWith('Bearer ')) {
      return res.status(401).json({
        error: 'Unauthorized',
        message: 'No token provided or invalid token format'
      });
    }
    
    const token = authHeader.split(' ')[1];
    const decoded = verifyToken(token);
    
    if (!decoded) {
      return res.status(401).json({
        error: 'Unauthorized',
        message: 'Invalid or expired token'
      });
    }
    
    // Verify user exists in database
    const { data: user, error } = await supabase
      .from('users')
      .select('id, email, created_at')
      .eq('id', decoded.userId)
      .single();
    
    if (error || !user) {
      return res.status(401).json({
        error: 'Unauthorized',
        message: 'User not found'
      });
    }
    
    // Attach user info to request
    req.user = {
      id: decoded.userId,
      username: decoded.username,
      email: user.email
    };
    
    next();
  } catch (error) {
    console.error('Authentication error:', error);
    return res.status(500).json({
      error: 'Authentication Failed',
      message: 'Internal server error during authentication'
    });
  }
};

const optionalAuthenticate = async (req, res, next) => {
  try {
    const authHeader = req.headers.authorization;
    
    if (authHeader && authHeader.startsWith('Bearer ')) {
      const token = authHeader.split(' ')[1];
      const decoded = verifyToken(token);
      
      if (decoded) {
        const { data: user } = await supabase
          .from('users')
          .select('id, email, created_at')
          .eq('id', decoded.userId)
          .single();
        
        if (user) {
          req.user = {
            id: decoded.userId,
            username: decoded.username,
            email: user.email
          };
        }
      }
    }
    
    next();
  } catch (error) {
    // Don't block request for optional auth
    next();
  }
};

module.exports = {
  authenticate,
  optionalAuthenticate
};