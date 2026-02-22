class ApiResponse {
  static success(res, data, message = 'Success', statusCode = 200) {
    return res.status(statusCode).json({
      success: true,
      message,
      data,
      timestamp: new Date().toISOString()
    });
  }
  
  static error(res, message = 'Error', statusCode = 400, errors = null) {
    const response = {
      success: false,
      message,
      timestamp: new Date().toISOString()
    };
    
    if (errors) {
      response.errors = errors;
    }
    
    return res.status(statusCode).json(response);
  }
  
  static created(res, data, message = 'Resource created successfully') {
    return this.success(res, data, message, 201);
  }
  
  static unauthorized(res, message = 'Unauthorized access') {
    return this.error(res, message, 401);
  }
  
  static forbidden(res, message = 'Forbidden') {
    return this.error(res, message, 403);
  }
  
  static notFound(res, message = 'Resource not found') {
    return this.error(res, message, 404);
  }
  
  static conflict(res, message = 'Resource already exists') {
    return this.error(res, message, 409);
  }
  
  static serverError(res, message = 'Internal server error') {
    return this.error(res, message, 500);
  }
}

module.exports = ApiResponse;