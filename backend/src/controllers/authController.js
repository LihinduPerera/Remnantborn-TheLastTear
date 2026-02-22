const { supabase, supabaseUser } = require('../config/database');
const { generateToken } = require('../config/auth');
const ApiResponse = require('../utils/response');

class AuthController {
  // Sign up with email and password
  async signup(req, res) {
    try {
      const { email, password, username } = req.body;
      
      // Validate input
      if (!email || !password || !username) {
        return ApiResponse.error(res, 'Email, password, and username are required', 400);
      }
      
      // Check if username exists
      const { data: existingProfile } = await supabase
        .from('profiles')
        .select('user_id')
        .eq('username', username)
        .single();
      
      if (existingProfile) {
        return ApiResponse.conflict(res, 'Username already taken');
      }
      
      // Create user in Supabase Auth
      const { data: authData, error: authError } = await supabaseUser.auth.signUp({
        email,
        password,
        options: {
          data: {
            username
          }
        }
      });
      
      if (authError) {
        return ApiResponse.error(res, `Signup failed: ${authError.message}`, 400);
      }
      
      const user = authData.user;
      
      // Create user record
      const { error: userError } = await supabase
        .from('users')
        .insert({
          id: user.id,
          email: user.email,
          created_at: new Date().toISOString()
        });
      
      if (userError) {
        // Rollback: delete auth user if user creation fails
        await supabase.auth.admin.deleteUser(user.id);
        return ApiResponse.error(res, 'Failed to create user record', 500);
      }
      
      // Create profile
      const { error: profileError } = await supabase
        .from('profiles')
        .insert({
          user_id: user.id,
          username: username,
          level: 1,
          remnant_count: 100,
          created_at: new Date().toISOString()
        });
      
      if (profileError) {
        // Rollback: delete user records if profile creation fails
        await supabase.from('users').delete().eq('id', user.id);
        await supabase.auth.admin.deleteUser(user.id);
        return ApiResponse.error(res, 'Failed to create user profile', 500);
      }
      
      // Generate JWT token
      const token = generateToken(user.id, username);
      
      // Update last login
      await supabase
        .from('users')
        .update({ last_login: new Date().toISOString() })
        .eq('id', user.id);
      
      return ApiResponse.created(res, {
        token,
        userId: user.id,
        username,
        email: user.email,
        level: 1,
        remnant_count: 100
      }, 'User created successfully');
      
    } catch (error) {
      console.error('Signup error:', error);
      return ApiResponse.serverError(res, 'Internal server error during signup');
    }
  }
  
  // Login with email and password
  async login(req, res) {
    try {
      const { email, password } = req.body;
      
      if (!email || !password) {
        return ApiResponse.error(res, 'Email and password are required', 400);
      }
      
      // Authenticate with Supabase
      const { data: authData, error: authError } = await supabaseUser.auth.signInWithPassword({
        email,
        password
      });
      
      if (authError) {
        return ApiResponse.unauthorized(res, 'Invalid email or password');
      }
      
      const user = authData.user;
      
      // Get user profile
      const { data: profile, error: profileError } = await supabase
        .from('profiles')
        .select('username, level, remnant_count')
        .eq('user_id', user.id)
        .single();
      
      if (profileError || !profile) {
        return ApiResponse.error(res, 'User profile not found', 404);
      }
      
      // Generate JWT token
      const token = generateToken(user.id, profile.username);
      
      // Update last login
      await supabase
        .from('users')
        .update({ last_login: new Date().toISOString() })
        .eq('id', user.id);
      
      await supabase
        .from('profiles')
        .update({ last_active: new Date().toISOString() })
        .eq('user_id', user.id);
      
      return ApiResponse.success(res, {
        token,
        userId: user.id,
        username: profile.username,
        level: profile.level,
        remnant_count: profile.remnant_count,
        email: user.email
      }, 'Login successful');
      
    } catch (error) {
      console.error('Login error:', error);
      return ApiResponse.serverError(res, 'Internal server error during login');
    }
  }
  
  // Verify token
  async verifyToken(req, res) {
    try {
      const authHeader = req.headers.authorization;
      
      if (!authHeader || !authHeader.startsWith('Bearer ')) {
        return ApiResponse.unauthorized(res, 'No token provided');
      }
      
      const token = authHeader.split(' ')[1];
      const { supabase } = require('../config/database');
      const { verifyToken } = require('../config/auth');
      
      const decoded = verifyToken(token);
      
      if (!decoded) {
        return ApiResponse.unauthorized(res, 'Invalid or expired token');
      }
      
      // Get user info
      const { data: user, error } = await supabase
        .from('users')
        .select(`
          id,
          email,
          profiles (
            username,
            level,
            remnant_count,
            avatar_url
          )
        `)
        .eq('id', decoded.userId)
        .single();
      
      if (error || !user) {
        return ApiResponse.unauthorized(res, 'User not found');
      }
      
      return ApiResponse.success(res, {
        userId: user.id,
        username: user.profiles.username,
        level: user.profiles.level,
        remnant_count: user.profiles.remnant_count,
        avatar_url: user.profiles.avatar_url,
        email: user.email
      }, 'Token is valid');
      
    } catch (error) {
      console.error('Token verification error:', error);
      return ApiResponse.serverError(res, 'Internal server error during token verification');
    }
  }
  
  // Development login bypass (for Unreal Engine testing)
  async devLogin(req, res) {
    try {
      const { email } = req.body;
      
      if (!email) {
        return ApiResponse.error(res, 'Email is required for dev login', 400);
      }
      
      if (process.env.NODE_ENV !== 'development') {
        return ApiResponse.forbidden(res, 'Dev login only available in development mode');
      }
      
      // Get user by email
      const { data: users, error: usersError } = await supabase
        .from('users')
        .select(`
          id,
          email,
          profiles (
            username,
            level,
            remnant_count
          )
        `)
        .eq('email', email);
      
      if (usersError || !users || users.length === 0) {
        return ApiResponse.notFound(res, 'User not found');
      }
      
      const user = users[0];
      
      if (!user.profiles) {
        return ApiResponse.error(res, 'User profile not found', 404);
      }
      
      // Generate JWT token
      const token = generateToken(user.id, user.profiles.username);
      
      // Update last login
      await supabase
        .from('users')
        .update({ last_login: new Date().toISOString() })
        .eq('id', user.id);
      
      await supabase
        .from('profiles')
        .update({ last_active: new Date().toISOString() })
        .eq('user_id', user.id);
      
      return ApiResponse.success(res, {
        token,
        userId: user.id,
        username: user.profiles.username,
        level: user.profiles.level,
        remnant_count: user.profiles.remnant_count,
        email: user.email
      }, 'Dev login successful');
      
    } catch (error) {
      console.error('Dev login error:', error);
      return ApiResponse.serverError(res, 'Internal server error during dev login');
    }
  }
}

module.exports = new AuthController();