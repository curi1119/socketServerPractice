package com.curi.net {
	import flash.events.Event;
	import flash.events.IOErrorEvent;
	import flash.events.ProgressEvent;
	import flash.events.SecurityErrorEvent;
	import flash.external.ExternalInterface;
	import flash.net.Socket;
	import flash.utils.ByteArray;

	public class ChatSocket {
		public static const JOIN_CMD:String  = 'JOIN';
		public static const SAY_CMD:String   = 'SAY';
		public static const LEAVE_CMD:String = 'LEAVE';

		private var _socket:Socket;
		private var _host:String;
		private var _port:int                = -1;
		private var _name:String;

		public function ChatSocket( host:String, port:int ) {
			_host = host;
			_port = port;
			_socket = new Socket();

			_socket.addEventListener( Event.CLOSE, closeHandler );
			_socket.addEventListener( Event.CONNECT, connectHandler );
			_socket.addEventListener( IOErrorEvent.IO_ERROR, ioErrorHandler );
			_socket.addEventListener( SecurityErrorEvent.SECURITY_ERROR, securityErrorHandler );
			_socket.addEventListener( ProgressEvent.SOCKET_DATA, socketDataHandler );
		}

		public function dispose():void {
			_host = null;
			_port = -1;

			_socket.removeEventListener( Event.CLOSE, closeHandler );
			_socket.removeEventListener( Event.CONNECT, connectHandler );
			_socket.removeEventListener( IOErrorEvent.IO_ERROR, ioErrorHandler );
			_socket.removeEventListener( SecurityErrorEvent.SECURITY_ERROR, securityErrorHandler );
			_socket.removeEventListener( ProgressEvent.SOCKET_DATA, socketDataHandler );
			_socket = null;
		}

		public function join( name:String ):void {
			_socket.connect( _host, _port );
			_name = name;
		}

		public function leave():void {
			leaveCmd();
			//_socket.close();
		}

		public function getMembers():void {
			// send GET_MEMBERS CMD
		}

		public function sendMessage( body:String ):void {
			debugTrace( "message sending...", body );
			sayCmd( body );
		}

		private function socketDataHandler( event:ProgressEvent ):void {
			debugTrace( "socketDataHandler: " + event );
			var bytes:ByteArray = new ByteArray();
			_socket.readBytes( bytes, event.bytesTotal );
			var msg:String = bytes.toString();
			switch ( true ) {
				default:
					ExternalInterface.call( 'recv', msg );
					break;
			}
		}

		public function joinCmd( name:String ):void {
			sendCmd( JOIN_CMD, name );
		}

		public function sayCmd( msg:String ):void {
			sendCmd( SAY_CMD, msg );
		}

		public function leaveCmd():void {
			sendCmd( LEAVE_CMD );
		}

		private function sendCmd( cmd:String, body:String = null ):void {
			var message:String = cmd;
			if ( body != null ) {
				message += ' ' + body;
			}
			message += '\n';

			var buf:ByteArray = new ByteArray;
			buf.writeUTFBytes( message );
			debugTrace( buf );
			_socket.writeBytes( buf );
			//_socket.writeUTFBytes( msg );
			_socket.flush();
		}

		private function connectHandler( event:Event ):void {
			debugTrace( "connectHandler: " + event );
			joinCmd( _name );
		}

		private function closeHandler( event:Event ):void {
			debugTrace( "closeHandler: " + event );
			dispose();
		}

		private function ioErrorHandler( event:IOErrorEvent ):void {
			debugTrace( "ioErrorHandler: " + event );
		}

		private function securityErrorHandler( event:SecurityErrorEvent ):void {
			debugTrace( "securityErrorHandler: " + event );
		}

		private function debugTrace( ... args ):void {
			var str:String = args.join( ' ' );
			trace( str );
			ExternalInterface.call( 'console.log', str );
		}

	}
}
