# app.rb
require 'sinatra/base'
require 'sinatra/json'
require 'aws-sdk'

module Tape
  class App < Sinatra::Base
    def bucket
      @bucket ||= Aws::S3::Resource.new(region: 'us-east-2').bucket('itmstore')
    end

    get '/' do
      erb :index
    end

    get '/uploads/new' do
      erb :upload
    end

    get '/uploads' do
      json objects: bucket.objects(delimiter: '/', prefix: 'todo/').map(&:key).keep_if { |obj| obj =~ /.+[^\/]$/i }
    end

    post '/uploads' do
      filename = params[:file][:filename]
      file = params[:file][:tempfile]

      if filename.end_with? '.wav', '.mp3'
        obj = bucket.object "todo/#{filename}"

        obj.upload_file file

        [200, 'ok']
      else
        [500, 'o no']
      end
    end
  end
end
